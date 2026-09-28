#include "crash1_widescreen.h"

#include "core.h"
#include "dynarec_dispatch.h"
#include "game.h"
#include "mods.h"

#include <cstdlib>
#include <lucent/log.h>
#include <utility>

namespace crash1 {
namespace {

// $a0 and $a1 carry the centre into set_geom_offset; 0x80042F8C shifts each left by 16 itself.
constexpr int kCentreXArgument = 4;
constexpr int kCentreYArgument = 5;
// set_geom_screen takes the projection-plane distance in $a0 (0x80042FAC stores it to CR[26]).
constexpr int kScreenDistanceArgument = 4;

// The GTE control-register numbers this owner reads back to prove what the guest published. Named
// here once because the meaning is what makes the check readable, not the number.
constexpr std::uint32_t kGteCrOfx = 24;
constexpr std::uint32_t kGteCrOfy = 25;
constexpr std::uint32_t kGteCrH = 26;

constexpr int kMaximumCentre = 0x7FFF; // the leaf's `sll 16` already truncates a 32-bit $a0

[[noreturn]] void refuse(const char *what) {
  lucent::error("crash1-wide", "SCUS-949.00 guest widescreen {}", what);
  std::abort();
}

Crash1Widescreen &ownerFrom(Core *core, const char *site) {
  if (!core || !core->runtime) {
    lucent::error("crash1-wide", "SCUS-949.00 {} override ran without its title runtime", site);
    std::abort();
  }
  // The policy is reached as a const base pointer because that is the framework's seam, so the
  // per-Core state behind it comes back through a checked downcast. A null or foreign result is a
  // wiring defect and stops the run rather than quietly presenting a 4:3 picture under a wide
  // claim.
  auto *const policy = dynamic_cast<Crash1Widescreen *>(
      const_cast<GuestWidescreenProjection *>(core->runtime->guestWidescreenProjection()));
  if (!policy) {
    lucent::error("crash1-wide", "SCUS-949.00 {} override reached another title's policy", site);
    std::abort();
  }
  return *policy;
}

void originalCentre(Core &core) {
  crash::dynarec::requireGuestReturn(crash::dynarec::callOriginal(core, kSetGeomOffset),
                                     "crash1-wide::centre original");
}

void originalInitProjection(Core &core) {
  crash::dynarec::requireGuestReturn(crash::dynarec::callOriginal(core, kProjectionInit),
                                     "crash1-wide::init projection original");
}

void originalScreenDistance(Core &core) {
  crash::dynarec::requireGuestReturn(crash::dynarec::callOriginal(core, kSetGeomScreen),
                                     "crash1-wide::screen distance original");
}

void centreOverride(Core *core) {
  ownerFrom(core, "set_geom_offset").publishCentre(*core, originalCentre);
}

void initProjectionOverride(Core *core) {
  ownerFrom(core, "gte_init").publishInitProjection(*core, originalInitProjection);
}

void screenDistanceOverride(Core *core) {
  ownerFrom(core, "set_geom_screen").observeScreenDistance(*core, originalScreenDistance);
}

} // namespace

Crash1Widescreen::Crash1Widescreen(Latch latch) : latch_(latch) {
  if (!latch_) {
    refuse("requires the shared plan latch");
  }
}

PresentationAspect Crash1Widescreen::presentationAspect(const Core &core) const {
  if (!core.game) {
    return PresentationAspect::Standard4x3;
  }
  // Mods is the one source of truth the player edits live, and `Mods::init` has already refused the
  // enhancements this widescreen-only title does not ship. ASPECT_AUTO is NOT folded to 16:9 here:
  // it resolves against the live sink inside the plan builder, so a headless run with no wide sink
  // correctly resolves to 4:3 instead of claiming a widening it did not perform.
  switch (core.game->mods.aspect) {
  case ASPECT_4_3:
    return PresentationAspect::Standard4x3;
  case ASPECT_16_9:
    return PresentationAspect::Wide16x9;
  case ASPECT_21_9:
    return PresentationAspect::UltraWide21x9;
  case ASPECT_AUTO:
    return PresentationAspect::MatchSink;
  default:
    lucent::error("crash1-wide", "invalid aspect selector {}", core.game->mods.aspect);
    std::abort();
  }
}

Crash1Widescreen &Crash1Widescreen::from(Core &core) {
  return ownerFrom(&core, "frame boundary");
}

bool Crash1Widescreen::isGuestRam(std::uint32_t address) {
  // A NULL is never a valid record, and physical address 0 is the BIOS/KSEG-aliased region rather
  // than a title-owned structure.
  constexpr std::uint32_t kKseg0Base = 0x80000000u;
  constexpr std::uint32_t kKseg1Base = 0xA0000000u;
  constexpr std::uint32_t kMainRamBytes = 0x00200000u;
  constexpr std::uint32_t kParallelRamBytes = 0x00100000u;

  if (address == 0) {
    return false;
  }
  if (address >= kKseg0Base && address - kKseg0Base < kMainRamBytes) {
    return true;
  }
  if (address >= kKseg1Base && address - kKseg1Base < kParallelRamBytes) {
    return true;
  }
  return address < kMainRamBytes;
}

GuestProjectionGeometry Crash1Widescreen::measuredGeometry(int displayWidth, int displayHeight) {
  // Crash 1's guest draw AREA is the PSX default whole-display-area: FUN_80042A04 stores
  // `(GPUSTAT & 0x3FFF) | 0xE1001000` at 0x80042A58, which is origin (0, 4) with width and height
  // zero, and on a PSX that IS the display area. So the title has no second, narrower clip
  // rectangle, and the one horizontal extent it does publish is the display extent it sent through
  // GP1 0xC0 - which the framework decodes and this owner reads rather than restating. That is why
  // all three of nativePresentation, nativeProjection.extent and nativeProjection.drawWidth are
  // the same measured number here, and it is a fact about this title rather than a shortcut.
  if (displayWidth <= 0 || displayHeight <= 0) {
    lucent::error("crash1-wide",
                  "SCUS-949.00 published no usable display extent ({}x{}); the title's draw area is "
                  "the display area, so refusing is better than guessing a 4:3 width",
                  displayWidth,
                  displayHeight);
    std::abort();
  }
  return {{displayWidth, displayHeight}, displayWidth};
}

std::int32_t Crash1Widescreen::widenedCentreX(std::int32_t retailCentreX, int margin) {
  // `retail + margin`, never `read + margin`. The leaf's argument is the TITLE's own value (the
  // camera-shake global DAT_8006193C), not a read-back of the register this owner widened, so the
  // widened centre can never be fed back in and compounded the way an accumulate-in-place owner
  // would. The saturation is the leaf's own `sll 16` truncation, kept here so a centre the guest
  // could not have produced refuses instead of wrapping into a plausible-looking frame.
  const std::int64_t widened = static_cast<std::int64_t>(retailCentreX) + margin;
  if (widened < -kMaximumCentre || widened > kMaximumCentre) {
    lucent::error("crash1-wide",
                  "widening the guest centre {} by {} leaves the representable 16.16 range",
                  retailCentreX,
                  margin);
    std::abort();
  }
  return static_cast<std::int32_t>(widened);
}

GuestProjectionPlan Crash1Widescreen::relatch(Core &core) {
  GuestProjectionPlan latched = latch_(&core, measuredGeometry(core.game->gpu.s_disp_w, core.game->gpu.s_disp_h));
  if (latched.projectionCenterX <= 0 || latched.guestDrawWidth <= 0 || latched.projectionCenterX > kMaximumCentre) {
    lucent::error("crash1-wide",
                  "the framework returned an unusable guest projection (centre={}, draw width={})",
                  latched.projectionCenterX,
                  latched.guestDrawWidth);
    std::abort();
  }
  plan_ = latched;
  return latched;
}

void Crash1Widescreen::publishCentre(Core &core, const RetailBody &retail) {
  if (!core.game) {
    refuse("set_geom_offset reached a Core with no Game");
  }
  if (!retail) {
    refuse("the centre publication requires the retail guest body");
  }

  // The title's own arguments, and the widening base. 0x80042F8C shifts each left by 16, so these
  // are whole pixels. The base is the value in the register RIGHT NOW and never a remembered one:
  // this leaf is a pure function of its arguments, the GTE register is never fed back into `$a0`, and
  // the argument is a live global (0x80017F00 passes DAT_8006193C, the camera-shake word). That is
  // what makes the widening idempotent by construction rather than by a guard - and it is also why
  // a remembered baseline is wrong: the title re-authors its centre per frame, so yesterday's value
  // is not retail, it is history.
  const auto retailX = static_cast<std::int32_t>(core.r[kCentreXArgument]);
  const auto retailY = static_cast<std::int32_t>(core.r[kCentreYArgument]);
  retail_ = {retailX, retailY};

  const GuestProjectionPlan latched = relatch(core);
  if (latched.widescreen()) {
    core.r[kCentreXArgument] = static_cast<std::uint32_t>(widenedCentreX(retailX, latched.projectionHorizontalMargin));
  } else {
    // 4:3 IDENTITY, by construction: the margin is zero, so the title's own argument reaches the
    // leaf untouched and the leaf's own `sll 16` produces retail's CR[24] bit for bit.
    core.r[kCentreXArgument] = static_cast<std::uint32_t>(retailX);
  }
  // The vertical centre is never moved. A vertical shift relocates every primitive without widening
  // anything, and the contract holds OFY and the vertical field of view fixed.
  core.r[kCentreYArgument] = static_cast<std::uint32_t>(retailY);

  // THE ARGUMENT MUST BE CAPTURED BEFORE THE LEAF RUNS, and this is the same defect the shared rule
  // in game/core/guest_projection_publication.cpp had. The guest's retail leaf is
  // `sll $a0, $a0, 0x10` -- it shifts the argument register IN PLACE, and $a0 is kCentreXArgument --
  // so reading `core.r[kCentreXArgument]` after `retail(core)` yields `centre << 16`. The guard then
  // compared a correctly-published 86 against 5,636,096 and aborted a frame whose widening was
  // exactly right. Invisible at 4:3, where retail's OFX is 0 and `0 == 0` passes.
  const std::int32_t expectedX = static_cast<std::int32_t>(core.r[kCentreXArgument]);

  retail(core);

  // What the GUEST published, read out of the coprocessor rather than assumed from the argument.
  // A vertical shift would move every primitive without widening anything, and a horizontal shift
  // that missed the plan's centre would be a translation under a wide claim - both are named.
  //
  // SIGNED 16.16, AND THE SIGN IS EXPLICIT. The leaf's `sll 16` on a 32-bit `$a0`/`$a1` leaves the
  // two's complement of a negative centre in the top halfword, and these registers are signed. A plain
  // `>> 16` on the unsigned read-back is a LOGICAL shift, so a centre below zero - which 0x80017F00
  // publishes whenever the camera-shake word goes negative - comes back as 65529 and this guard would
  // abort a perfectly good frame. Found by the Crash 2/3 owners' shared-rule test, which publishes a
  // negative centre on purpose.
  const auto publishedX = static_cast<std::int32_t>(static_cast<std::int16_t>(gte_read_ctrl(kGteCrOfx) >> 16));
  const auto publishedY = static_cast<std::int32_t>(static_cast<std::int16_t>(gte_read_ctrl(kGteCrOfy) >> 16));
  if (publishedY != retail_.y) {
    lucent::error("crash1-wide",
                  "the guest published OFY {} from an unchanged $a1 = {}; a vertical shift moves "
                  "the picture without widening it",
                  publishedY,
                  retail_.y);
    std::abort();
  }
  if (publishedX != expectedX) {
    lucent::error("crash1-wide",
                  "the guest published OFX {} but $a0 was {}; the plan and the guest's own leaf "
                  "disagree, so the frame would not be the widening it claims",
                  publishedX,
                  expectedX);
    std::abort();
  }
  if (latched.widescreen() && retailX != 0) {
    // A non-zero centre is the camera-shake word DAT_8006193C, republished per frame at
    // 0x80017F00. The widening has to ride it, and this line is what says it did.
    lucent::info("crash1-wide",
                 "guest centre {} -> {} (retail {} + margin {}, OFY {}, H {}), host canvas {} "
                 "(native {})",
                 retailX,
                 publishedX,
                 retailX,
                 latched.projectionHorizontalMargin,
                 publishedY,
                 publishedScreenDistance_,
                 latched.presentationExtent.width,
                 latched.nativeExtent.width);
  }
  published_ = true;
}

void Crash1Widescreen::publishInitProjection(Core &core, const RetailBody &retail) {
  if (!core.game) {
    refuse("gte_init reached a Core with no Game");
  }
  if (!retail) {
    refuse("the init projection publication requires the retail guest body");
  }
  retail(core);

  // 0x80042B68 publishes H = 0x3E8 and 0x80042B88/0x80042B8C publish OFX = OFY = 0 from `$zero`, so
  // this body never passes through the leaf and there is no argument register to move. Reading the
  // tuple back out of the coprocessor is what makes the retail 4:3 baseline a MEASURED fact instead
  // of a constant this file happens to know, and comparing it against the manifest turns a changed
  // title into a named refusal. The widening itself rides the per-frame leaf at 0x80042F8C, which
  // the core loop reaches at 0x800123BC every frame, so no frame is left un-widened by relying on it
  // and no coprocessor register is written from here.
  publishedScreenDistance_ = static_cast<std::int32_t>(gte_read_ctrl(kGteCrH) & 0xFFFF);
  const std::int32_t retailX = static_cast<std::int32_t>(static_cast<std::int16_t>(gte_read_ctrl(kGteCrOfx) >> 16));
  const std::int32_t retailY = static_cast<std::int32_t>(static_cast<std::int16_t>(gte_read_ctrl(kGteCrOfy) >> 16));
  if (publishedScreenDistance_ != kRetailScreenDistance || retailX != kRetailCentreX || retailY != kRetailCentreY) {
    lucent::error("crash1-wide",
                  "gte_init published H {} OFX {} OFY {}; the manifest records H {} OFX {} OFY {}",
                  publishedScreenDistance_,
                  retailX,
                  retailY,
                  kRetailScreenDistance,
                  kRetailCentreX,
                  kRetailCentreY);
    std::abort();
  }
  // Recorded, not remembered: the per-frame leaf re-authors the centre every frame, so this is the
  // value the init publication produced and nothing more.
  retail_ = {retailX, retailY};
  relatch(core);
  lucent::info("crash1-wide",
               "guest projection init published H {}, OFX {}, OFY {} — the retail 4:3 baseline this "
               "owner widens; host canvas {} (native {})",
               publishedScreenDistance_,
               retailX,
               retailY,
               plan_.presentationExtent.width,
               plan_.nativeExtent.width);
}

void Crash1Widescreen::observeScreenDistance(Core &core, const RetailBody &retail) {
  if (!core.game) {
    refuse("set_geom_screen reached a Core with no Game");
  }
  if (!retail) {
    refuse("the screen-distance publication requires the retail guest body");
  }
  // H is the SCALE, and a widening holds it fixed, so this site changes nothing: the retail body
  // runs on the title's own `$a0` and the value it published is recorded. Overriding it is what
  // turns "the owner never touches H" from an intention into a checked fact.
  retail(core);
  publishedScreenDistance_ = static_cast<std::int32_t>(core.r[kScreenDistanceArgument] & 0xFFFF);
}

void installCrash1Widescreen(Core &core) {
  const struct Binding {
    std::uint32_t address;
    const char *owner;
    psx::cpu::NativeFunction function;
  } bindings[]{
      {kSetGeomOffset, "Crash SetGeomOffset", centreOverride},
      {kProjectionInit, "Crash GTE projection init", initProjectionOverride},
      {kSetGeomScreen, "Crash SetGeomScreen", screenDistanceOverride},
  };
  for (const Binding &binding : bindings) {
    if (!crash::dynarec::installOverride(core, binding.address, binding.owner, binding.function)) {
      std::abort();
    }
  }
  lucent::info("crash1-wide",
               "guest widescreen installed: SetGeomOffset 0x{:08X}, gte_init 0x{:08X}, "
               "SetGeomScreen 0x{:08X}; retail OFX {} OFY {} H {}",
               kSetGeomOffset,
               kProjectionInit,
               kSetGeomScreen,
               kRetailCentreX,
               kRetailCentreY,
               kRetailScreenDistance);
}

} // namespace crash1
