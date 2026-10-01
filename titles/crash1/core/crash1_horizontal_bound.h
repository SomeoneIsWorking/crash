// Crash Bandicoot 1 (SCUS_949.00) — the title's own HORIZONTAL BOUND owner.
//
// WHAT THIS IS FOR, IN ONE LINE. `crash1_widescreen.*` widens the frame by moving OFX and holding H,
// and its safety rests on two measured facts: no guest branch reads a projection coprocessor
// register, and no horizontal cull exists to clip the newly revealed geometry. The first is a
// census of `cfc2` readers and is complete. The second was NOT a census: the prior survey scanned
// 72,192 instruction words for a compare against a 4:3 dot width, found none, and correctly
// recorded that a cull against a VARIABLE bound carries no immediate and is invisible to that scan.
// This owner closes that gap from the other end — it owns the variable the guest actually uses as
// its horizontal bound, and it checks the bound at the point where the title turns it into a
// visibility decision.
//
// THE BOUND IS NOT A COPROCESSOR REGISTER. The guest keeps it in a main-RAM global, 0x800578D0,
// which is OUTSIDE the executable's own text segment: SCUS_949.00 is 290,816 bytes = 0x800 header
// + 0x46800 text and nothing else, so this is loader-created zero-initialised memory the title
// reaches with `lui $reg,0x8005`. There is exactly one store to it in the whole image, at
// 0x80017820 (`sw $v0,0x78D0($at)`, word 0xAC2278D0) inside the camera setup FUN_80017790, which is
// where the per-camera-mode H is chosen: 0x25->500, 0x1E->960, 0x38->800, 0x3C->460, 0x5A->288, and
// gte_init publishes 0x3E8 = 1000 once at boot. The per-frame re-send is at 0x80026770
// (`jal 0x80042FAC`, word 0x0C010BEB) in FUN_80026650, fed by `lw $a0,0x78D0($a0)` at 0x80026764 —
// so FUN_80026650 is a CONSUMER that re-sends the global into CR[26] every frame, not a producer.
//
// TWENTY READERS, AND NONE OF THEM CARRIES AN IMMEDIATE. The enumeration reached them three ways,
// and names what each could not reach: 17 sites by a lui+displacement word
// scan, 3 more by a `lw $rt,0($rN)` form whose register a nearby lui/addiu pair proves, and 1 more
// (0x8001DFFC, 229 instructions past its `lui $s5,0x8005` at 0x8001DF64) only by naming that pair.
// Every one of the twenty is `lui 0x8005` + a 16-bit displacement, which is exactly why the earlier
// literal-immediate cull census reported none of them.
//
// AND NONE OF THEM IS A SCREEN-SPACE CULL. Decompiling the two that turn the bound into a decision
// answers the question the scan could not:
//
//   FUN_8003A144 [0x8003A144,0x8003A76C) — the pre-GTE object lighting setup. It takes the bound as
//   its 5th stack argument (`lw $v0,0x14($sp)`, 0x8003A18C, word 0x8FA20014) and rejects the object
//   when NOT (H < Z) at 0x8003A240/0x8003A244, then when 11999 < Z at 0x8003A248/0x8003A24C, where Z
//   is the projected depth from the `rtps` at 0x8003A220 (cop2 0x49E012). So the band is
//   H < Z <= 11999 and H is the GTE NEAR-PLANE DISTANCE. That is the projection-plane distance used
//   as a depth threshold, which is what a PSX H is; it is not a screen-space half-width.
//
//   FUN_8001DE78 [0x8001DE78,0x8001E3D4) — the pre-GTE object submitter, `GoolObjectTransform`. It
//   reads the bound at 0x8001DF6C and 0x8001DFFC through `$s5`, proved equal to 0x800578D0 by
//   0x8001DF64/0x8001DF68, halves it at 0x8001E008..0x8001E010, and passes
//   `(object+0x138) + 0x800 - H/2` (assembled at 0x8001E004/0x8001E014) to FUN_8003A76C as its 5th
//   argument. FUN_8003A76C uses that argument as `param_5*4 + (IR0.r+IR0.g+IR0.b >> 5)*-4`: a GTE
//   LIGHT-INTENSITY offset, not an extent.
//
// SO THE WIDENING'S CONTRACT, STATED AS SOMETHING CHECKED. A widening holds H fixed, which means
// every one of the twenty readers must still read a retail H while the frame is wide. This owner
// enforces that at the submitter: it reads the bound the submitter is about to consume, runs the
// retail body, and refuses if the submitter moved the bound or if the value it consumed is not one
// the measured camera setup can publish. If a later widening owner were to widen by scaling H — the
// textbook approach, and wrong for this title, whose screen centring is already inside the view
// matrix — this owner fires on the first object instead of the picture quietly changing shape.
//
// WHAT THIS OWNER DOES NOT ESTABLISH, because a guard that overstates itself is worse than none.
// It is not a horizontal-cull proof: it covers the two functions that turn the bound into a decision
// and the twenty sites that read it, and it has NOT decompiled the other eighteen. It does not
// change any pixel: it reads guest state and observes. It does not establish a wide leg in a run —
// no disc media is provisioned on this machine, so no live run has reached the submitter yet. And it
// is not a native producer, so it does not advance S005: `RenderCapabilities::widescreenOnly()`
// stays the honest profile for this title, and `interpolatedNative()` remains blocked on a native
// render path and a `TemporalSceneSource` this repository does not have.
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
