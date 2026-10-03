// Crash Bandicoot 1 (SCUS-949.00) - the title's own HORIZONTAL BOUND owner.
//
// `crash1_widescreen.*` widens the frame by moving OFX and holding H, and its safety rests on two
// measured facts: no guest branch reads a projection coprocessor register, and no horizontal cull
// clips the newly revealed geometry. The first is a complete census of `cfc2` readers. The second
// was NOT: a scan of all 72,192 instruction words for a compare against a 4:3 dot width finds none,
// and correctly so, because a cull against a VARIABLE bound carries no immediate and is invisible
// to that scan. This owner closes that gap from the other end. It owns the variable the guest
// actually uses as its horizontal bound, and checks it where the title turns it into a decision.
//
// THE BOUND IS NOT A COPROCESSOR REGISTER. The guest keeps it in a main-RAM global, 0x800578D0,
// outside the executable's own text: SCUS_949.00 is 290,816 bytes = 0x800 header + 0x46800 text and
// nothing else. There is exactly one store to it in the whole image, at 0x80017820 inside the camera
// setup FUN_80017790, which is where the per-camera-mode H is chosen; gte_init publishes 0x3E8 =
// 1000 once at boot. The per-frame re-send at 0x80026770 is a CONSUMER, not a producer. Its twenty
// readers are all `lui` plus a 16-bit displacement, which is exactly why the literal-immediate cull
// census reported none of them.
//
// NONE OF THOSE READERS IS A SCREEN-SPACE CULL. Decompiling the two that turn the bound into a
// decision answers what the scan could not:
//
//   FUN_8003A144 [0x8003A144,0x8003A76C) takes the bound as its 5th stack argument and rejects the
//   object when NOT (H < Z <= 11999), where Z is the projected depth. So the band is
//   H < Z <= 11999 and H is the GTE NEAR-PLANE DISTANCE, not a screen-space half-width.
//
//   FUN_8001DE78 [0x8001DE78,0x8001E3D4), the pre-GTE object submitter, halves the bound and passes
//   `(object+0x138) + 0x800 - H/2` to FUN_8003A76C as its 5th argument, which is a GTE
//   LIGHT-INTENSITY offset rather than an extent.
//
// SO THE WIDENING'S CONTRACT IS CHECKED. A widening holds H fixed, so every one of the twenty readers
// must still read a retail H while the frame is wide. This owner enforces that at the submitter: it
// reads the bound the submitter is about to consume, runs the retail body, and refuses if the
// submitter moved the bound or if the value it consumed is not one the measured camera setup can
// publish. A widening by scaling H would fire this on the first object instead of quietly changing
// the shape of the picture.
//
// WHAT THIS OWNER DOES NOT ESTABLISH, because a guard that overstates itself is worse than none. It
// is not a horizontal-cull proof: it covers the two functions that turn the bound into a decision, and
// it has NOT decompiled the other eighteen. It changes no pixel; it reads guest state and observes.
#pragma once

#include "guest_widescreen_projection.h"

#include <cstdint>
#include <vector>

class Core;

namespace crash1 {

// The bound, its single writer, and its per-frame re-send. Same pattern as `crash1_widescreen.h`:
// the literal lives here and the authority is titles/crash1/executable.json.
inline constexpr std::uint32_t kHorizontalBound = 0x800578D0u;
inline constexpr std::uint32_t kHorizontalBoundWriter = 0x80017820u;
inline constexpr std::uint32_t kHorizontalBoundResend = 0x80026770u;

// The two consumers, one body each, and the far limit the near-plane arm materialises as 0x2EE0.
// 0x2EE0 = 12000 is the value 0x8003A248 builds, and the arm that uses it is `11999 < Z -> reject`,
// so the accepted band is Z < 12000: the bound is EXCLUSIVE at the far end.
inline constexpr std::uint32_t kHorizontalSubmitter = 0x8001DE78u;
inline constexpr std::uint32_t kHorizontalSubmitterEnd = 0x8001E3D4u;
inline constexpr std::uint32_t kHorizontalNearPlane = 0x8003A144u;
inline constexpr std::uint32_t kHorizontalNearPlaneEnd = 0x8003A76Cu;
inline constexpr std::int32_t kHorizontalFarLimit = 12000;

// gte_init 0x80042B1C publishes CR[26] = 0x3E8 = 1000 once, before any camera mode is chosen.
inline constexpr std::int32_t kRetailInitScreenDistance = 1000;

// The H values the guest can publish, from the ONE store at 0x80017820 in the camera setup
// FUN_80017790 (per-camera mode -> H) and the once-per-boot gte_init publication of 0x3E8. A bound
// outside this set is not a value retail can produce, so seeing one means something moved it.
// Public and pure so the test pins this set against the manifest rather than against a copy.
[[nodiscard]] constexpr bool isRetailHorizontalBound(std::int32_t bound) noexcept {
  return bound == kRetailInitScreenDistance || bound == 500 || bound == 960 || bound == 800 || bound == 460 ||
         bound == 288;
}

// The title's own bound. It reads PRE-GTE game state — the main-RAM global the camera setup wrote —
// runs the retail submitter, and records the bound that submitter consumed. It writes no guest
// state: a producer that wrote guest state would be a defect, and this one is a guard on the
// contract the widening depends on, not a second copy of the submitter.
class Crash1HorizontalBound final {
public:
  // An authenticated original guest body, executed through Lightrec. Injected so a test can observe
  // the consumption without a guest image, exactly as `Crash1Widescreen::RetailBody`.
  using RetailBody = void (*)(Core &);

  // Record the bound the retail submitter is about to read, run it, and check the contract.
  void observeSubmitter(Core &core, const RetailBody &retail);

  // Every distinct bound a submission has consumed, in first-seen order. The denominator a run
  // reports alongside it is `submissions()`: a set of one with many submissions is a different
  // statement from a set of one with one submission, and only the second is a fact about the title.
  [[nodiscard]] const std::vector<std::int32_t> &observedBounds() const {
    return observedBounds_;
  }

  [[nodiscard]] std::uint64_t submissions() const {
    return submissions_;
  }

  // The bound the last submission consumed, for the cross-owner check against what the guest
  // published into CR[26]. Zero until a submission has run.
  [[nodiscard]] std::int32_t lastObservedBound() const {
    return lastObservedBound_;
  }

  // The measured GTE near-plane predicate, as a pure function of the bound and a projected depth.
  // This is the decision FUN_8003A144 makes at 0x8003A240/0x8003A244 and 0x8003A248/0x8003A24C; it
  // is exposed so a test can pin the band without a guest image, and it is the ONLY place in this
  // repository that writes the rule down. A widening holds the bound, so `insideNearPlane` must
  // answer identically before and after one runs.
  [[nodiscard]] static constexpr bool insideNearPlane(std::int32_t bound, std::int32_t depth) noexcept {
    return bound < depth && depth < kHorizontalFarLimit;
  }

  // The measured light-intensity offset the submitter derives from half the bound, as a pure
  // function. `(object+0x138) + 0x800 - H/2` is assembled at 0x8001E004/0x8001E014 with the halving
  // done as `srl/sra` sign-extension at 0x8001E008/0x8001E010, so this reproduces the title's own
  // rounding toward zero rather than a C division that would differ on a negative bound.
  [[nodiscard]] static constexpr std::int32_t lightIntensityOffset(std::int32_t objectValue,
                                                                   std::int32_t bound) noexcept {
    return (objectValue + 0x800) - (bound / 2);
  }

  // Whether a submission left the bound intact, as the pure decision the owner applies. It is
  // exposed because the owner's other decision - "is this a value the camera setup can publish" - is
  // already `isRetailHorizontalBound`, and a guard whose two halves cannot both be exercised is a
  // guard with an untested half. The owner refuses when this is false.
  [[nodiscard]] static constexpr bool boundIntact(std::int32_t consumed, std::int32_t after) noexcept {
    return consumed == after;
  }

  // This title's owner, reached from a Core that is running it. Same idiom and same reason as
  // `Crash1Widescreen::from`: the policy is reached as a const base pointer through the framework's
  // seam, so the per-Core state comes back through a checked downcast and a foreign result is a
  // named refusal rather than a silent no-op.
  static Crash1HorizontalBound &from(Core &core);

private:
  std::vector<std::int32_t> observedBounds_;
  std::int32_t lastObservedBound_{};
  std::uint64_t submissions_{};
};

// Install this title's horizontal-bound consumer on one Core. It is reached from the runtime's
// registerOverrides, next to the three projection sites, because the two owners share one
// measurement: `crash1_widescreen.*` holds H fixed and this owner checks that it stayed fixed.
void installCrash1HorizontalBound(Core &core);

} // namespace crash1
