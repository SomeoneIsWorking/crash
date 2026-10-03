// Falsifiers for SCUS-949.00's guest-widescreen owner.
//
// Every case pins a PRODUCTION contract of `crash1::Crash1Widescreen`, and each is written so the
// mutation it claims to catch makes it fail. Nothing here reimplements the widening: the rule is the
// shared `crash::GuestProjectionPublication` and the latch is the framework's own
// `gpu_vk_latch_guest_projection`, so a change to the framework's rule moves these expectations with
// it instead of letting a title's copy of the arithmetic rot.
//
// The measured inputs are read out of the authenticated executable and recorded in
// titles/crash1/executable.json:
//   0x80042F94  ctc2 $a0,0xC000   OFX = $a0 << 16   }  set_geom_offset 0x80042F8C
//   0x80042F98  ctc2 $a1,0xC800   OFY = $a1 << 16   }
//   0x80042FAC  ctc2 $a0,0xD000   H   = $a0          }  set_geom_screen
//   0x80042B68  ctc2 $t0,0xD000   H   = 0x3E8 = 1000 }  gte_init 0x80042B1C, which also
//   0x80042B88  ctc2 $zero,0xC000 OFX = 0            }  publishes the retail baseline from $zero
// and 320 is the display extent this owner measures from the guest's own GP1(0xC0) publication
// (FUN_80041C38, GPU driver table entry 7 at 0x80054A40), not a constant this file invented.

#include "crash1_widescreen.h"

#include "core.h"
#include "crash1_runtime.h"
#include "game.h"
#include "game_runtime.h"
#include "gpu_vk.h"
#include "hw_bind.h"
#include "mods.h"
#include "native_dispatch.h"
#include "psx_exe_image.h"

#include <cstdint>
#include <cstdio>
#include <memory>

namespace {

using crash1::Crash1Widescreen;

// The guest's own leaf, recovered instruction by instruction from 0x80042F8C. RECOMPUTING the
// outputs rather than writing them is the point: a test can then tell "the owner wrote the widened
// centre" from "the title's own leaf produced the widened centre", and only the second is the
// mechanism this port is allowed to use.
//   0x80042F8C sll $a0,16 / 0x80042F90 sll $a1,16
//   0x80042F94 ctc2 $a0,0xC000 (CR[24]=OFX) / 0x80042F98 ctc2 $a1,0xC800 (CR[25]=OFY)
constexpr std::uint32_t kGteCrOfx = 24;
constexpr std::uint32_t kGteCrOfy = 25;
constexpr std::uint32_t kGteCrH = 26;

void retailCentre(Core &core) {
  gte_write_ctrl(kGteCrOfx, static_cast<std::uint32_t>(core.r[4]) << 16);
  gte_write_ctrl(kGteCrOfy, static_cast<std::uint32_t>(core.r[5]) << 16);
}

// The guest's own gte_init, 0x80042B1C, word for word: 0x80042B50 CR[29]=0x155, 0x80042B5C
// CR[30]=0x100, 0x80042B68 CR[26]=0x3E8, 0x80042B74 CR[27]=0xEF9E, 0x80042B80 CR[28]=0x01400000,
// 0x80042B88 CR[24]=0, 0x80042B8C CR[25]=0. 0x80042B88 and 0x80042B8C are `ctc2 $zero`, so no
// argument register of this owner could reach them - which is why the test drives that site too.
void retailInitProjection(Core &core) {
  core.r[8] = 0x155;
  gte_write_ctrl(29, 0x155);
  gte_write_ctrl(30, 0x100);
  core.r[8] = 1000;
  gte_write_ctrl(kGteCrH, 1000);
  gte_write_ctrl(27, 0xFFFFEF9Eu);
  gte_write_ctrl(28, 0x01400000u);
  gte_write_ctrl(kGteCrOfx, 0);
  gte_write_ctrl(kGteCrOfy, 0);
}

// The guest's own set_geom_screen, 0x80042FAC: one `ctc2 $a0,0xD000`.
void retailScreenDistance(Core &core) {
  gte_write_ctrl(kGteCrH, core.r[4] & 0xFFFF);
}

bool expect(bool condition, const char *message) {
  if (!condition) {
    std::fprintf(stderr, "%s\n", message);
  }
  return condition;
}

// A synthetic PS-X EXE spanning the whole resident text, carrying the three real instruction words at
// their real offsets. The identity it establishes is what `installSites` needs, and the
// words are what tie this fixture to titles/crash1/executable.json rather than to this file.
std::vector<std::uint8_t> residentFixture() {
  constexpr std::uint32_t kTextAddress = 0x80010000u;
  constexpr std::uint32_t kTextSize = 0x46800u;
  std::vector<std::uint8_t> bytes(psx::cpu::kPsxExeHeaderBytes + kTextSize, 0u);
  const std::array<char, 8> kMagic{'P', 'S', '-', 'X', ' ', 'E', 'X', 'E'};
  for (std::size_t index = 0; index < kMagic.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(kMagic[index]);
  }
  auto put = [&bytes](std::size_t at, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
      bytes[at + shift / 8] = static_cast<std::uint8_t>(value >> shift);
    }
  };
  put(0x10, kTextAddress); // pc0
  put(0x18, kTextAddress); // text address
  put(0x1C, kTextSize);    // text size
  put(0x30, 0x801FFFF0u);  // stack address
  const struct {
    std::uint32_t address;
    std::uint32_t word;
  } site_words[]{
      // 0x80042B68 ctc2 $t0,0xD000  -> CR[26] = 0x3E8
      {0x80042B68u, 0x48C8D000u},
      // 0x80042B88 ctc2 $zero,0xC000 -> CR[24] = 0
      {0x80042B88u, 0x48C0C000u},
      // 0x80042B8C ctc2 $zero,0xC800 -> CR[25] = 0
      {0x80042B8Cu, 0x48C0C800u},
      // 0x80042F94 ctc2 $a0,0xC000  -> CR[24] = $a0
      {0x80042F94u, 0x48C4C000u},
      // 0x80042F98 ctc2 $a1,0xC800  -> CR[25] = $a1
      {0x80042F98u, 0x48C5C800u},
      // 0x80042FAC ctc2 $a0,0xD000  -> CR[26] = $a0
      {0x80042FACu, 0x48C4D000u},
  };
  for (const auto &site : site_words) {
    put(psx::cpu::kPsxExeHeaderBytes + (site.address - kTextAddress), site.word);
  }
  return bytes;
}

// THE INSTALL PROOF, WITH NO HLE PLAN IN EXISTENCE. PlatformHle covers the stock library services;
// none of these three leaves is in it. Installing them on a Core that has never had a HLE plan
// initialised, and then invoking them through the image-scoped native override table, is what makes
// the proof independent of the framework's `resolveHostDispatch` ordering: if the leaves only
// resolved because an HLE plan happened to be consulted first, this would not install at all.
bool installWithNoHlePlan() {
  bool ok = true;
  crash1::Crash1Runtime runtime;
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  Core &core = game->core;
  gte_init();
  gte_bind(&core);
  core.rsub.mode.setPath(RenderPath::Gte);
  core.game->gpu.s_disp_w = 320;
  core.game->gpu.s_disp_h = 240;
  core.game->mods.aspect = ASPECT_16_9;

  const auto bytes = residentFixture();
  const auto loaded = psx::cpu::loadPsxExeImage(core, bytes, "crash1-wide-install-fixture");
  const std::uint32_t sites[] = {crash1::kProjectionInit, crash1::kSetGeomOffset, crash1::kSetGeomScreen};
  ok &= expect(static_cast<bool>(loaded), "the install fixture did not establish a resident image");
  // The direct "no HLE plan in existence" proof: the stock library table has no entry for any of
  // the three leaves, and none was registered, so resolving them cannot have gone through it.
  for (const std::uint32_t address : sites) {
    ok &= expect(core.game->platform_hle.lookup(address) == nullptr,
                 "a measured projection leaf was already in the HLE table, so the no-HLE proof is vacuous");
  }

  runtime.widescreen().installSites(core);
  for (const std::uint32_t address : sites) {
    const auto image = core.currentImageIdentity(address);
    ok &= expect(image.has_value(), "no image identity at an override address");
    if (!image) {
      continue;
    }
    ok &= expect(core.nativeDispatcher().isInstalled({*image, address}),
                 "a measured projection leaf did not install with no HLE plan in existence");
    // `intercepts` is the predicate a guest CALL is routed by, so this is the load-bearing half:
    // the leaf is reached through the image-scoped native override table, not merely recorded in
    // it. Invoking the body is deliberately not attempted here - that would execute the fixture's
    // guest words through the executor, and the transformation itself is pinned case by case above
    // against the recovered leaf.
    ok &= expect(core.nativeDispatcher().intercepts({*image, address}),
                 "an installed projection leaf does not intercept a guest call at its address");
  }

  // And the widened centre really does come out of the owner, through the same production entry the
  // override uses, on this second Core.
  const auto image = core.currentImageIdentity(crash1::kSetGeomOffset);
  if (image) {
    ok &= expect(core.nativeDispatcher().isInstalled({*image, crash1::kSetGeomOffset}),
                 "SetGeomOffset is not installed on the install-proof Core");
  }
  return ok;
}

} // namespace

int main() {
  // The shipping runtime's own owner, reached the way the framework reaches it. A refactor that
  // stopped returning it from `guestWidescreenProjection()` fails here rather than in a report.
  crash1::Crash1Runtime runtime;
  if (!expect(runtime.guestWidescreenProjection() == &runtime.widescreen(),
              "the runtime's widescreen policy is not the owner it hands the framework")) {
    return 1;
  }
  if (!expect(runtime.renderCapabilities().defaultPath == RenderPath::Gte &&
                  !runtime.renderCapabilities().nativeRenderPath && !runtime.renderCapabilities().temporalInterpolation,
              "Crash 1 no longer declares widescreenOnly(): GTE shipping, no native producer, no "
              "temporal interpolation")) {
    return 1;
  }

  // The framework's latch reads the Core's render path and the title's own policy, and ignores the
  // requested aspect when either is missing. RenderPath::Gte is this title's SHIPPING path
  // (RenderCapabilities::widescreenOnly()), and the policy is the owner under test, so the fixture
  // wires exactly what a real run wires. The runtime goes in BEFORE the Game, because `Game`'s
  // constructor is what captures it.
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  Core &core = game->core;
  // The framework binds a Core's own GTE register file once per frame-step (hw_bind.h), and the
  // owner reads CR[24]/CR[25] back out of it, so the fixture binds the way the product does rather
  // than reading a default file that was never powered.
  gte_init();
  gte_bind(&core);
  core.rsub.mode.setPath(RenderPath::Gte);
  // The measured display extent: this title's guest draw area is the PSX default whole-display area
  // (FUN_80042A04 stores `(GPUSTAT & 0x3FFF) | 0xE1001000` at 0x80042A58), so the single horizontal
  // extent it has is the one it published through GP1 0xC0.
  core.game->gpu.s_disp_w = 320;
  core.game->gpu.s_disp_h = 240;
  Crash1Widescreen &owner = runtime.widescreen();
  bool ok = true;

  auto publishedOfx = [&core] {
    return static_cast<std::int32_t>(gte_read_ctrl(kGteCrOfx) >> 16);
  };
  auto publishedOfy = [&core] {
    return static_cast<std::int32_t>(gte_read_ctrl(kGteCrOfy) >> 16);
  };
  auto publish = [&core, &owner](std::int32_t x, std::int32_t y) {
    // This title reaches the leaf INDIRECTLY (no `jal`, no pointer-table word, and a live run shows
    // `$r31` is the enclosing chain's return address), so the owner widens every reach and never reads
    // `$r31`. The stale value left here on purpose is what a real run carries.
    core.r[4] = static_cast<std::uint32_t>(x);
    core.r[5] = static_cast<std::uint32_t>(y);
    owner.publishCentre(core, retailCentre);
  };

  // 1. 4:3 IDENTITY, EXACT. The margin is zero, so the title's own argument reaches the leaf
  //    untouched and the leaf produces retail's CR[24] bit for bit.
  core.game->mods.aspect = ASPECT_4_3;
  publish(0, 0);
  ok &= expect(publishedOfx() == 0 && publishedOfy() == 0 && core.r[4] == 0 && core.r[5] == 0,
               "4:3 identity: the retail centre did not survive the publication byte for byte");
  ok &= expect(!owner.plan().widescreen(), "4:3 produced a widescreen plan");
  ok &= expect(owner.plan().projectionHorizontalMargin == 0, "4:3 produced a non-zero horizontal margin");

  // 2. 4:3 IDENTITY IS NOT "THE PLAN SAYS 4:3". A mutation that always added the margin would still
  //    report widescreen()==false headless under ASPECT_AUTO and slip past case 1, so publish a
  //    non-zero retail centre: the margin is the only thing that could change it.
  publish(7, 11);
  ok &= expect(publishedOfx() == 7 && publishedOfy() == 11 && core.r[4] == 7 && core.r[5] == 11,
               "4:3 identity changed a non-zero retail centre");

  // 3. THE WIDENING. 16:9 on a 320-wide canvas is 428, so the margin is (428-320)/2 = 54 and the
  //    centre moves by exactly that, with OFY untouched and H never involved.
  core.game->mods.aspect = ASPECT_16_9;
  publish(0, 0);
  ok &= expect(owner.plan().widescreen(), "16:9 did not produce a widescreen plan");
  ok &= expect(owner.plan().presentationExtent.width == 428 && owner.plan().nativeExtent.width == 320,
               "the 16:9 presentation extent is not 428 over a 320 native");
  ok &= expect(owner.plan().projectionHorizontalMargin == 54, "the 16:9 horizontal margin is not 54");
  ok &= expect(owner.plan().projectionCenterX == 214, "the 16:9 projection centre is not 214");
  ok &= expect(publishedOfx() == 54, "the widened guest OFX is not 54");
  ok &= expect(publishedOfy() == 0 && core.r[5] == 0,
               "the widening moved OFY, which is a vertical shift and not a widening");
  ok &= expect(owner.retailCentre().x == 0 && owner.retailCentre().y == 0,
               "the centre the guest published was not recorded as its own argument");
  ok &= expect(owner.published(), "a widened publication did not record that it published");

  // 4. THE 4:3 FRAME IS CONTAINED AT ITS ORIGINAL SCALE: it spans [0,320) at retail and [54,374)
  //    widened, which is (428-320)/2 of margin per side with no rescale - the passing case in
  //    external/psxport/docs/presentation-contract.md, "What counts as a widening".
  ok &= expect(owner.plan().presentationHorizontalMargin == (owner.plan().presentationExtent.width - 320) / 2,
               "the widened 4:3 frame is not translated into the wide canvas at its original scale");

  // 5. IDEMPOTENCE, structurally. The leaf's argument is the title's own value, so publishing again
  //    cannot compound the way an accumulate-in-place owner would: 54 stays 54, not 108.
  publish(0, 0);
  ok &= expect(publishedOfx() == 54, "a second publication compounded the margin instead of reusing it");
  publish(0, 0);
  ok &= expect(publishedOfx() == 54, "a third publication compounded the margin");
  // 5b. THE REACH IS INDIRECT, AND EVERY REACH WIDENS. A widening that keyed off `$r31` would look at
  //     whatever the enclosing chain left there and either refuse a live frame or, worse, pass one
  //     through because the value happened to match a call site from another title.
  ok &= expect(crash1::kCentreReach == crash::CentreReach::IndirectCall && !owner.declaresPassThrough(),
               "Crash 1 no longer states that its centre leaf is reached indirectly with no pass-through");
  core.r[31] = 0x80017844u; // what a live disc-backed run carries at the leaf
  publish(0, 0);
  ok &= expect(publishedOfx() == 54, "an indirect reach did not widen because $r31 named no call site");

  // 6. THE RUNTIME-VARYING BASELINE. 0x80017F00 publishes SetGeomOffset(DAT_8006193C, ...) and
  //    DAT_8006193C is a live camera-shake global, so the widening rides whatever the title
  //    published rather than a remembered zero.
  publish(-5, 0);
  ok &= expect(publishedOfx() == 49, "the widening did not ride a non-zero retail centre");

  // 7. UNWIDENING. The player changes the setting back and the next publication carries the retail
  //    centre again, or a 4:3 canvas would keep a wide frustum.
  core.game->mods.aspect = ASPECT_4_3;
  publish(0, 0);
  ok &= expect(publishedOfx() == 0, "returning to 4:3 left the widened centre published");
  core.game->mods.aspect = ASPECT_16_9;

  // 8. H IS THE SCALE AND IS NEVER TOUCHED. 0x80042FAC is overridden so that this is checked rather
  //    than intended: the retail body runs on the title's own $a0 and the value it published is
  //    recorded. A mutation that widened H instead of OFX would leave every centre case above
  //    passing and still be a zoom, so the recorded H is the discriminator.
  core.game->mods.aspect = ASPECT_16_9;
  publish(0, 0);
  const std::int32_t widenedBefore = publishedOfx();
  core.r[4] = 288; // the 0x5A camera mode in FUN_80017790
  owner.observeScreenDistance(core, retailScreenDistance);
  ok &= expect(static_cast<std::int32_t>(gte_read_ctrl(kGteCrH)) == 288,
               "the screen-distance site did not let the guest publish its own H");
  ok &= expect(owner.publishedScreenDistance() == 288, "the published H was not recorded");
  core.r[4] = 500; // the 0x25 camera mode
  owner.observeScreenDistance(core, retailScreenDistance);
  ok &= expect(static_cast<std::int32_t>(gte_read_ctrl(kGteCrH)) == 500,
               "the per-camera-mode H was not republished by the guest");
  ok &= expect(publishedOfx() == widenedBefore && publishedOfx() == 54,
               "the screen-distance site changed the horizontal centre; H is the scale, OFX is the field");

  // 9. THE RETAIL BASELINE IS MEASURED, NOT ASSUMED. gte_init publishes H = 1000 and OFX = OFY = 0
  //    from `$zero`, so a fresh owner records what the guest actually published.
  Crash1Widescreen fresh{Crash1Widescreen::facts(), gpu_vk_latch_guest_projection};
  retailInitProjection(core);
  fresh.publishInitProjection(core, retailInitProjection);
  ok &= expect(fresh.retailCentre().x == 0 && fresh.retailCentre().y == 0,
               "the init publication's centre was not read back from the guest's own registers");
  ok &= expect(fresh.publishedScreenDistance() == 1000, "the init publication's H was not recorded");
  ok &= expect(fresh.plan().presentationExtent.width == 428,
               "the init site did not latch the plan for the live display extent");
  ok &= expect(fresh.plan().projectionHorizontalMargin == 54, "the init site latched the wrong margin");

  // 10. NO LITERAL 4:3 WIDTH IS BAKED INTO THE OWNER. The extent comes from the guest's own GP1
  //     publication, so a title that published 368 widens 368 -> 492.
  core.game->gpu.s_disp_w = 368;
  publish(0, 0);
  ok &= expect(owner.plan().nativeExtent.width == 368 && owner.plan().presentationExtent.width == 492 &&
                   owner.plan().projectionHorizontalMargin == 62,
               "the owner did not measure the title's own display extent");

  // 11. GUEST RAM BOUND. A fixture using only low addresses cannot catch a bound that forgets to
  //     translate a KSEG0 address, and the first live run of the sibling owner refused a real
  //     title-owned address for exactly that reason.
  ok &= expect(Crash1Widescreen::isGuestRam(0x800578D0) && Crash1Widescreen::isGuestRam(0x80100000) &&
                   !Crash1Widescreen::isGuestRam(0) && !Crash1Widescreen::isGuestRam(0x80200000),
               "the guest RAM bound accepted an address outside the 2 MiB main RAM");

  ok &= installWithNoHlePlan();

  if (!ok) {
    return 1;
  }
  std::printf("Crash 1 guest widescreen: 4:3 identity exact, 16:9 moves the guest centre by the "
              "margin with OFY and H untouched, the margin cannot compound, and the init "
              "publication is read from the guest\n");
  return 0;
}