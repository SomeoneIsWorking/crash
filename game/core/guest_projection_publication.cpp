#include "guest_projection_publication.h"

#include "core.h"
#include "game.h"
#include "mods.h"

#include <cstdlib>
#include <lucent/log.h>

namespace crash {
namespace {

// $a0 and $a1 carry the centre into set_geom_offset; the leaf shifts each left by 16 itself, so these
// are whole pixels. `set_geom_screen` takes the projection-plane distance in $a0.
constexpr int kCentreXArgument = 4;
constexpr int kCentreYArgument = 5;
constexpr int kScreenDistanceArgument = 4;

// $r31 holds the address after the `jal` that reached the leaf, so the call site is one below it.
// This is exact for a `jal` and is the only form these leaves are reached by: a resident-word scan
// of each image finds no pointer-table entry equal to any of the three entries.
constexpr int kReturnAddressRegister = 31;

// The leaf's own `sll 16` already truncates a 32-bit $a0, so a centre the guest could not have
// produced is a refusal rather than a wrapped one.
constexpr int kMaximumCentre = 0x7FFF;

} // namespace

GuestProjectionPublication::GuestProjectionPublication(const ProjectionTitleFacts &facts, Latch latch)
    : facts_(facts), latch_(latch) {
  if (!latch_) {
    lucent::error(facts_.serial, "guest widescreen requires the shared plan latch");
    std::abort();
  }
  if (facts_.projectionInit == 0 || facts_.setGeomOffset == 0 || facts_.setGeomScreen == 0) {
    lucent::error(facts_.serial,
                  "guest widescreen has no measured publication entry; refusing rather than guessing a "
                  "guest address");
    std::abort();
  }
  if (facts_.centreCallSites == nullptr || facts_.centreCallSiteCount == 0) {
    lucent::error(facts_.serial,
                  "guest widescreen has no measured centre call sites; an override reached through a "
                  "reach this repository did not measure cannot be widened safely");
    std::abort();
  }
}

PresentationAspect GuestProjectionPublication::presentationAspect(const Core &core) const {
  if (!core.game) {
    return PresentationAspect::Standard4x3;
  }
  // Mods is the one source of truth the player edits live, and `Mods::init` has already refused the
  // enhancements a widescreen-only title does not ship. ASPECT_AUTO is NOT folded to 16:9 here: it
  // resolves against the live sink inside the plan builder, so a headless run with no wide sink
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
    lucent::error(facts_.serial, "invalid aspect selector {}", core.game->mods.aspect);
    std::abort();
  }
}

bool GuestProjectionPublication::isGuestRam(std::uint32_t address) {
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

GuestProjectionGeometry GuestProjectionPublication::measuredGeometry(int displayWidth, int displayHeight) const {
  // The guest draw AREA of every Crash title measured here is the PSX default whole-display area:
  // `(GPUSTAT & 0x3FFF) | 0xE1001000`, which is origin (0, 4) with width and height zero, and on a
  // PSX that IS the display area. So the title has no second, narrower clip rectangle, and the one
  // horizontal extent it does publish is the display extent it sent through GP1 0xC0 - which the
  // framework decodes and this owner reads rather than restating. That is why all three of
  // nativePresentation, nativeProjection.extent and nativeProjection.drawWidth are the same measured
  // number here, and it is a fact about these titles rather than a shortcut.
  if (displayWidth <= 0 || displayHeight <= 0) {
    lucent::error(facts_.serial,
                  "the guest published no usable display extent ({}x{}); the title's draw area is the "
                  "display area, so refusing is better than guessing a 4:3 width",
                  displayWidth,
                  displayHeight);
    std::abort();
  }
  return {{displayWidth, displayHeight}, displayWidth};
}

std::int32_t GuestProjectionPublication::widenedCentreX(std::int32_t retailCentreX, int margin) const {
  // `retail + margin`, never `read + margin`. The leaf's argument is the TITLE's own value (at the
  // per-frame publication, a live camera global), not a read-back of the register this owner
  // widened, so the widened centre can never be fed back in and compounded the way an
  // accumulate-in-place owner would. The saturation is the leaf's own `sll 16` truncation, kept here
  // so a centre the guest could not have produced refuses instead of wrapping into a plausible frame.
  const std::int64_t widened = static_cast<std::int64_t>(retailCentreX) + margin;
  if (widened < -kMaximumCentre || widened > kMaximumCentre) {
    lucent::error(facts_.serial,
                  "widening the guest centre {} by {} leaves the representable 16.16 range",
                  retailCentreX,
                  margin);
    std::abort();
  }
  return static_cast<std::int32_t>(widened);
}

bool GuestProjectionPublication::passesThrough(std::uint32_t callSite) const {
  for (std::size_t index = 0; index < facts_.passThroughCallSiteCount; ++index) {
    if (facts_.passThroughCallSites[index] == callSite) {
      return true;
    }
  }
  return false;
}

bool GuestProjectionPublication::knowsCallSite(std::uint32_t callSite) const {
  for (std::size_t index = 0; index < facts_.centreCallSiteCount; ++index) {
    if (facts_.centreCallSites[index] == callSite) {
      return true;
    }
  }
  return false;
}

std::uint32_t GuestProjectionPublication::observedCallSite(const Core &core, const char *site) const {
  const auto returnAddress = static_cast<std::uint32_t>(core.r[kReturnAddressRegister]);
  if (returnAddress < 4) {
    lucent::error(facts_.serial, "{} override reached with $r31 = 0x{:08X}", site, returnAddress);
    std::abort();
  }
  const std::uint32_t callSite = returnAddress - 4;
  if (knowsCallSite(callSite)) {
    return callSite;
  }
  // A call site this repository never measured is the one case where widening would be a guess: it
  // could be a site whose argument already carries a previous widening, and nothing here knows. So it
  // is a named refusal rather than a default.
  lucent::error(facts_.serial,
                "{} override was reached from 0x{:08X}, which the title manifest does not name; this "
                "repository will not widen a call site whose argument provenance it has not measured",
                site,
                callSite);
  std::abort();
}

GuestProjectionPlan GuestProjectionPublication::relatch(Core &core) {
  GuestProjectionPlan latched = latch_(&core, measuredGeometry(core.game->gpu.s_disp_w, core.game->gpu.s_disp_h));
  if (latched.projectionCenterX <= 0 || latched.guestDrawWidth <= 0 || latched.projectionCenterX > kMaximumCentre) {
    lucent::error(facts_.serial,
                  "the framework returned an unusable guest projection (centre={}, draw width={})",
                  latched.projectionCenterX,
                  latched.guestDrawWidth);
    std::abort();
  }
  plan_ = latched;
  return latched;
}

void GuestProjectionPublication::publishCentre(Core &core, const RetailBody &retail) {
  if (!core.game) {
    lucent::error(facts_.serial, "the centre publication reached a Core with no Game");
    std::abort();
  }
  if (!retail) {
    lucent::error(facts_.serial, "the centre publication requires the retail guest body");
    std::abort();
  }
  const std::uint32_t callSite = observedCallSite(core, "set_geom_offset");
  const bool widen = !passesThrough(callSite);

  // The title's own arguments, and the widening base. The leaf shifts each left by 16, so these are
  // whole pixels. The base is the value in the register RIGHT NOW and never a remembered one: this
  // leaf is a pure function of its arguments, the GTE register is never fed back into `$a0` at a
  // widening call site, and the per-frame publication's argument is a live global. That is what makes
  // the widening idempotent by construction rather than by a guard - and it is also why a remembered
  // baseline is wrong: the title re-authors its centre, so yesterday's value is not retail, it is
  // history.
  const auto retailX = static_cast<std::int32_t>(core.r[kCentreXArgument]);
  const auto retailY = static_cast<std::int32_t>(core.r[kCentreYArgument]);
  retail_ = {retailX, retailY};

  const GuestProjectionPlan latched = relatch(core);
  if (latched.widescreen() && widen) {
    core.r[kCentreXArgument] = static_cast<std::uint32_t>(widenedCentreX(retailX, latched.projectionHorizontalMargin));
  } else {
    // 4:3 IDENTITY, by construction: the margin is zero, so the title's own argument reaches the leaf
    // untouched and the leaf's own `sll 16` produces retail's CR[24] bit for bit. A pass-through site
    // takes this arm under a wide plan too, for the measured reason recorded in the manifest.
    core.r[kCentreXArgument] = static_cast<std::uint32_t>(retailX);
  }
  // The vertical centre is never moved. A vertical shift relocates every primitive without widening
  // anything, and the contract holds OFY and the vertical field of view fixed.
  core.r[kCentreYArgument] = static_cast<std::uint32_t>(retailY);

  retail(core);

  // What the GUEST published, read out of the coprocessor rather than assumed from the argument. A
  // vertical shift would move every primitive without widening anything, and a horizontal shift that
  // missed the plan's centre would be a translation under a wide claim - both are named.
  //
  // SIGNED 16.16, AND THE SIGN IS EXPLICIT. The leaf's `sll 16` on a 32-bit `$a0`/`$a1` leaves the
  // two's complement of a negative centre in the top halfword, and these registers are signed. A
  // plain `>> 16` on the unsigned read-back is a LOGICAL shift, so a camera-shake centre below zero
  // comes back as 65529 instead of -7 and this guard aborts a perfectly good frame. The int16_t
  // round-trip is what the register actually holds.
  const auto publishedX =
      static_cast<std::int32_t>(static_cast<std::int16_t>(gte_read_ctrl(facts_.centreXRegister) >> 16));
  const auto publishedY =
      static_cast<std::int32_t>(static_cast<std::int16_t>(gte_read_ctrl(facts_.centreYRegister) >> 16));
  const std::int32_t expectedX = static_cast<std::int32_t>(core.r[kCentreXArgument]);
  if (publishedY != retail_.y) {
    lucent::error(facts_.serial,
                  "the guest published the vertical centre {} from an unchanged $a1 = {}; a vertical "
                  "shift moves the picture without widening it",
                  publishedY,
                  retail_.y);
    std::abort();
  }
  if (publishedX != expectedX) {
    lucent::error(facts_.serial,
                  "the guest published the horizontal centre {} but $a0 was {}; the plan and the "
                  "guest's own leaf disagree, so the frame would not be the widening it claims",
                  publishedX,
                  expectedX);
    std::abort();
  }
  if (latched.widescreen() && widen && retailX != 0) {
    // A non-zero centre is the camera-shake word the per-frame publication re-authors. The widening
    // has to ride it, and this line is what says it did.
    lucent::info(facts_.serial,
                 "guest centre {} -> {} (retail {} + margin {}, vertical {}, H {}), host canvas {} "
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
  if (widen) {
    ++publications_;
  } else {
    ++passThroughs_;
  }
}

void GuestProjectionPublication::publishInitProjection(Core &core, const RetailBody &retail) {
  if (!core.game) {
    lucent::error(facts_.serial, "the projection init reached a Core with no Game");
    std::abort();
  }
  if (!retail) {
    lucent::error(facts_.serial, "the init projection publication requires the retail guest body");
    std::abort();
  }
  retail(core);

  // The init publication writes H and the two centres from registers inline, so it never passes
  // through the leaf and there is no argument register to move. Reading the tuple back out of the
  // coprocessor is what makes the retail 4:3 baseline a MEASURED fact instead of a constant this file
  // happens to know, and comparing it against the manifest turns a changed title into a named refusal.
  publishedScreenDistance_ = static_cast<std::int32_t>(gte_read_ctrl(facts_.screenDistanceRegister) & 0xFFFF);
  const std::int32_t retailX =
      static_cast<std::int32_t>(static_cast<std::int16_t>(gte_read_ctrl(facts_.centreXRegister) >> 16));
  const std::int32_t retailY =
      static_cast<std::int32_t>(static_cast<std::int16_t>(gte_read_ctrl(facts_.centreYRegister) >> 16));
  if (publishedScreenDistance_ != facts_.retailScreenDistance || retailX != facts_.retailCentreX ||
      retailY != facts_.retailCentreY) {
    lucent::error(facts_.serial,
                  "the projection init published H {} centre {} {}; the manifest records H {} centre "
                  "{} {}",
                  publishedScreenDistance_,
                  retailX,
                  retailY,
                  facts_.retailScreenDistance,
                  facts_.retailCentreX,
                  facts_.retailCentreY);
    std::abort();
  }
  // Recorded, not remembered: the per-frame leaf re-authors the centre, so this is the value the init
  // publication produced and nothing more.
  retail_ = {retailX, retailY};
  relatch(core);
  lucent::info(facts_.serial,
               "guest projection init published H {}, centre {} {} - the retail 4:3 baseline this owner "
               "widens; host canvas {} (native {})",
               publishedScreenDistance_,
               retailX,
               retailY,
               plan_.presentationExtent.width,
               plan_.nativeExtent.width);
}

void GuestProjectionPublication::observeScreenDistance(Core &core, const RetailBody &retail) {
  if (!core.game) {
    lucent::error(facts_.serial, "the screen-distance publication reached a Core with no Game");
    std::abort();
  }
  if (!retail) {
    lucent::error(facts_.serial, "the screen-distance publication requires the retail guest body");
    std::abort();
  }
  // H is the SCALE, and a widening holds it fixed, so this site changes nothing: the retail body runs
  // on the title's own `$a0` and the value it published is recorded. Overriding it is what turns
  // "the owner never touches H" from an intention into a checked fact. It is also where the near
  // plane and the HUD rectangle scalar in this title are protected, so it is the site whose silence
  // matters most.
  retail(core);
  publishedScreenDistance_ = static_cast<std::int32_t>(core.r[kScreenDistanceArgument] & 0xFFFF);
}

} // namespace crash
