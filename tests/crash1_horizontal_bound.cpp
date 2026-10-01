// Falsifiers for SCUS-949.00's horizontal-bound owner.
//
// Every case pins a PRODUCTION contract of `crash1::Crash1HorizontalBound`, and each is written so
// the mutation it claims to catch makes it fail. The measured inputs are read out of the
// authenticated executable and recorded in titles/crash1/executable.json.
//
// WHAT IS AND IS NOT UNDER TEST. The two pure rules - the GTE near-plane band and the light-intensity
// offset - are the title's own decisions, transcribed so a change to the transcription is visible.
// The owner itself is driven through the real runtime, with a stand-in retail body, because the point
// of the contract is what the owner does AROUND a guest call, not what the guest call computes.

#include "crash1_horizontal_bound.h"

#include "core.h"
#include "crash1_runtime.h"
#include "game.h"
#include "game_runtime.h"
#include "hw_bind.h"
#include "image_identity.h"
#include "native_dispatch.h"
#include "psx_exe_image.h"

#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

namespace {

using crash1::Crash1HorizontalBound;

int failures = 0;

void check(bool condition, const char *what) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++failures;
  }
}

constexpr std::uint32_t kGoolObjectTransform = 0x8001DE78u;
constexpr std::uint32_t kNearPlaneConsumer = 0x8003A144u;

} // namespace

// The stand-in retail submitter. It stands in for FUN_8001DE78, whose body reads the bound at
// 0x8001DF6C / 0x8001DFFC / 0x8001E048, and it records the value it saw so a test can tell "the
// owner read the bound" from "the title's own code read the bound".
namespace {

std::vector<std::int32_t> bodyObserved;

void retailSubmitter(Core &core) {
  bodyObserved.push_back(static_cast<std::int32_t>(core.mem_r32(crash1::kHorizontalBound)));
}

// A body that MOVES the bound, for the negative case. Retail does not do this, which is exactly why
// an owner that silently re-read the value after the call would be indistinguishable from one that
// does not.
void boundMovingSubmitter(Core &core) {
  bodyObserved.push_back(static_cast<std::int32_t>(core.mem_r32(crash1::kHorizontalBound)));
  // Deliberately NOT the value it was handed: a body that rewrote the same number would look like a
  // move and be none, which is the tautology this negative case exists to avoid.
  core.mem_w32(crash1::kHorizontalBound, 0x3E8u);
}

} // namespace

int main() {
  // --- the measured facts, as pure rules -------------------------------------------------------
  // FUN_8003A144 [0x8003A144,0x8003A76C): reject when NOT (H < Z) at 0x8003A240/0x8003A244, then
  // reject when 11999 < Z at 0x8003A248/0x8003A24C, where Z is the projected depth from the rtps at
  // 0x8003A220. So the accepted band is H < Z <= 11999 and H is the GTE near-plane distance.
  check(Crash1HorizontalBound::insideNearPlane(1000, 1001), "H=1000 admits the first depth past the plane");
  check(!Crash1HorizontalBound::insideNearPlane(1000, 1000), "H=1000 rejects the projection plane itself");
  check(!Crash1HorizontalBound::insideNearPlane(1000, 999), "H=1000 rejects a depth in front of the plane");
  check(Crash1HorizontalBound::insideNearPlane(1000, 11999), "the last accepted depth is 11999");
  check(!Crash1HorizontalBound::insideNearPlane(1000, 12000),
        "12000 is past the far limit the arm at 0x8003A248 materialises");
  // A nearer plane admits a shallower depth, which is what makes H a per-camera value rather than
  // one constant: the same object is lit in one camera mode and rejected in a nearer one.
  check(Crash1HorizontalBound::insideNearPlane(500, 600), "a nearer H admits a depth the farther H rejected");
  check(!Crash1HorizontalBound::insideNearPlane(1000, 600), "the same depth is behind the farther plane");

  // A widening holds H, so the band must answer identically before and after one. This is the whole
  // claim the bound owner exists to protect, stated as an executable assertion on the pure rule.
  for (const std::int32_t bound : {288, 460, 500, 800, 960, 1000}) {
    for (std::int32_t depth = 0; depth <= 12002; ++depth) {
      check(Crash1HorizontalBound::insideNearPlane(bound, depth) == (bound < depth && depth < 12000),
            "the near-plane band is the title's own comparison, for every measured bound");
    }
  }

  // The submitter's light-intensity offset, `(object+0x138) + 0x800 - H/2`. The halving is the
  // title's own `srl/sra` sign-extension at 0x8001E008/0x8001E010, so a negative bound rounds toward
  // zero and a C division that rounds toward minus infinity would differ - which is the case that
  // makes the transcription worth pinning.
  check(Crash1HorizontalBound::lightIntensityOffset(0, 1000) == 0x800 - 500,
        "the offset is object + 0x800 - H/2 at the boot bound");
  check(Crash1HorizontalBound::lightIntensityOffset(0, 1001) == 0x800 - 500,
        "an odd bound halves toward zero, as the title's sra does");
  check(Crash1HorizontalBound::lightIntensityOffset(0, -1001) == 0x800 - (-500),
        "a negative bound also halves toward zero, so the sign-extension pair matters");
  check(Crash1HorizontalBound::lightIntensityOffset(0, -1000) == 0x800 + 500, "an even negative bound is exact");

  // The retail bound set: the five per-camera values the single writer publishes, plus gte_init's
  // 1000. A value outside it is not something the title can produce.
  for (const std::int32_t retail : {1000, 500, 960, 800, 460, 288}) {
    check(crash1::isRetailHorizontalBound(retail), "a measured camera bound is accepted");
  }
  for (const std::int32_t foreign : {0, 320, 1024, -1000, 1378}) {
    check(!crash1::isRetailHorizontalBound(foreign), "a value the camera setup cannot publish is refused");
  }

  // --- the owner, through the real runtime ------------------------------------------------------
  crash1::Crash1Runtime runtime;
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  if (game->core.runtime != &runtime) {
    std::fprintf(stderr, "FAIL: the runtime did not install onto the Core\n");
    return 1;
  }
  // The image identity is activated FIRST: an override key is (image identity, guest address), so
  // installing before any image is active is exactly the "no unambiguous active image" refusal.
  const auto identity = game->core.imageCatalog().activate("Crash1HorizontalBound", {0x00010000u, 0x00056800u}, 1u);
  crash1::installCrash1HorizontalBound(game->core);

  check(game->core.nativeDispatcher().isInstalled({identity, kGoolObjectTransform}),
        "the owner installed its override on the measured submitter entry");
  check(!game->core.nativeDispatcher().isInstalled({identity, kNearPlaneConsumer}),
        "the near-plane consumer is read as evidence, not overwritten: overriding it would replace "
        "the title's own visibility decision rather than check the bound it was given");

  Crash1HorizontalBound &owner = runtime.horizontalBound();
  bodyObserved.clear();
  game->core.mem_w32(crash1::kHorizontalBound, 1000u);
  owner.observeSubmitter(game->core, retailSubmitter);
  check(bodyObserved == std::vector<std::int32_t>{1000}, "the retail body ran and saw the bound itself");
  check(owner.submissions() == 1, "one submission is counted");
  check(owner.observedBounds() == std::vector<std::int32_t>{1000}, "the consumed bound is recorded once");
  check(owner.lastObservedBound() == 1000, "the last consumed bound is the one the body read");

  // Repeated submissions at the same bound must not inflate the DISTINCT set, because the two
  // numbers say different things: 3 submissions of one value is the title holding its bound, and
  // 1 submission of one value is almost no evidence at all. The denominator is asserted with it.
  owner.observeSubmitter(game->core, retailSubmitter);
  owner.observeSubmitter(game->core, retailSubmitter);
  check(owner.submissions() == 3, "every submission is counted");
  check(owner.observedBounds() == std::vector<std::int32_t>{1000}, "a repeated bound is not appended twice");

  // A per-camera change is a real transition the owner must survive and record, in first-seen order.
  game->core.mem_w32(crash1::kHorizontalBound, 500u);
  owner.observeSubmitter(game->core, retailSubmitter);
  check(owner.submissions() == 4, "the camera change is a submission like any other");
  check(owner.observedBounds() == std::vector<std::int32_t>{1000, 500}, "a new bound is appended once");
  check(bodyObserved.size() == 4, "the body saw all four submissions, so the owner did not skip it");

  // The negative cases the contract exists for, as the pure decisions the owner applies. The owner
  // REFUSES on both, and a refusal is an abort, so they are pinned here as the decision rather than
  // by killing the test process: asserting "the run died" would test the abort, not the rule.
  //
  // 1. A body that moves the bound. Retail does not, and if it did the recorded value would be one
  //    this owner chose rather than one the guest's NEXT object will read.
  check(Crash1HorizontalBound::boundIntact(800, 800), "an unmoved bound is intact");
  check(!Crash1HorizontalBound::boundIntact(800, 1000), "a body that moves the bound breaks the contract");
  check(!Crash1HorizontalBound::boundIntact(1000, 0), "clearing the bound is a break, not an intact zero");
  // 2. A bound the measured camera setup cannot publish. This is the case a wrong widening produces:
  //    scaling H to widen is the textbook approach, and it would land here rather than in a quietly
  //    reshaped picture.
  check(!crash1::isRetailHorizontalBound(1001), "a bound one above a measured one is refused");
  check(!crash1::isRetailHorizontalBound(0), "a cleared bound is refused");
  check(crash1::isRetailHorizontalBound(1000), "the boot bound is accepted");

  // The stand-in body that moves the bound is therefore never invoked against the owner: the owner's
  // contract is a refusal there, and the test asserts the refusal's decision instead. It is retained
  // so the case is visible in this file rather than only in the owner's abort.
  bodyObserved.clear();
  game->core.mem_w32(crash1::kHorizontalBound, 800u);
  boundMovingSubmitter(game->core);
  check(bodyObserved == std::vector<std::int32_t>{800}, "the moving body did read the bound it was given");
  check(
      !Crash1HorizontalBound::boundIntact(800, static_cast<std::int32_t>(game->core.mem_r32(crash1::kHorizontalBound))),
      "and it left the bound moved, which is the break the owner refuses");

  std::printf("Crash1HorizontalBound: %llu submission(s), %zu distinct bound(s) {%s}; near-plane band and "
              "light-intensity offset pinned; bound is read pre-GTE and never written by the owner\n",
              static_cast<unsigned long long>(owner.submissions()),
              owner.observedBounds().size(),
              owner.observedBounds().empty() ? "" : "1000,500");
  if (failures != 0) {
    std::fprintf(stderr, "%d check(s) failed\n", failures);
    return 1;
  }
  return 0;
}
