// The Crash trilogy's ONE guest-projection widening rule, and the measured per-title facts it needs.
//
// WHY THIS IS IN `game/` AND NOT IN A TITLE. The three North American Crash titles publish their
// projection through the same three guest leaves, and the leaves are byte-for-byte identical between
// them where they can be compared: `set_geom_offset` and `set_geom_screen` have the SAME body sha256
// in SCUS-941.54 and SCUS_942.44 (`9aa95b09…` and `d8c79a8c…`, recorded in each title's manifest),
// and both titles' `gte_init` publish the same retail tuple H=1000, OFX=0, OFY=0, ZSF3=0x155,
// ZSF4=0x100, DQA=0xEF9E, DQB=0x01400000 from the same instruction shapes. That is direct evidence
// about the binaries, not franchise lineage, which is the bar `docs/codemap.md` sets for `game/`.
//
// WHAT THE RULE IS, AND WHY IT IS NOT A ZOOM. Beetle's `gte.c` names CR[24]=OFX, CR[25]=OFY,
// CR[26]=H, and its RTPS computes `h_div_sz = Divide(H, Z_FIFO(3))` then `TransformXY`, so a
// projected point lands at `SX = OFX + (H * IR1) / SZ`: OFX is the pixel centre and H is the scale.
// The visible world half-width at depth `pz` is therefore `(displayWidth/2 - OFX) * pz / H`. Retail
// is displayWidth 320 with OFX 0, so the visible half-width is `160*pz/H`. A 428-wide canvas holding
// that same 4:3 frame at its ORIGINAL pixel scale is the same equation with `OFX = (428-320)/2 = 54`
// and H unchanged. Substituting a smaller H would be a zoom. So the rule is: move OFX, hold OFY, hold
// H. The margin itself is NOT computed here — it comes from the framework's own latch, through
// `guest_projection_plan`, so this repository keeps no second copy of that arithmetic.
//
// WHY H IS HELD, WHICH IS THE PART THAT IS PER-TITLE AND MEASURED. In all three titles H is the GTE
// projection-plane distance, and a PSX H is also the near plane: a consumer rejects a vertex when NOT
// (H < Z < 12000). In Crash 2 and Crash 3 H is additionally a 2D overlay rectangle's height, from
// `(H * fog * 0xAA >> 20) - 0x6C`, clamped to 0..216. Raising H there would cull near geometry AND
// resize a HUD rectangle. `observeScreenDistance` exists so that "the owner never touches H" is a
// checked fact rather than an intention.
//
// WHY A CALL-SITE POLICY EXISTS, WHICH IS THE ONE PLACE THE THREE TITLES DIFFER. The widening's base
// is the title's own `$a0` as it stands right now, which is idempotent ONLY while the value arriving
// at the leaf does not already carry the margin. Crash 1 and Crash 2 have ZERO `cfc2` control reads
// of CR[24]/CR[25] anywhere in their images, so nothing can feed the register back. Crash 3 has one:
// `FUN_8004f6e4` at 0x8004F6E4 reads both registers out and `FUN_8001cd80` hands the result straight
// back to `set_geom_offset` at 0x8001D09C. Widening that call site would add the margin again on
// every pass of that path. So a title with a read-back names the call sites that must pass through
// unchanged, and the owner refuses any call site it was not told about rather than widening a site
// whose argument provenance nobody measured.
//
// The call site is recovered from `$r31 - 4`, which is exact for a `jal` (it sets `$ra = pc + 4`).
// Every measured call site of every measured leaf in both titles is a `jal`, and a resident-word scan
// of each image finds ZERO pointer-table entries equal to any of the three entries — so no indirect
// reach can make `$r31` something this owner has not been told about. That is why an unknown call
// site is a refusal and not a guess.
#pragma once

#include "guest_widescreen_projection.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

class Core;
class Game;

namespace crash {

// The measured per-title projection facts. Every member was read out of that title's authenticated
// executable by `tools/probe_title_projection.py` and is recorded in `titles/<title>/executable.json`
// under `runtime.projection`; the probe diffs the constants this repository compiles against that
// manifest, so the two cannot drift. Nothing here is a tuned value.
struct ProjectionTitleFacts {
  // For the log line and the refusal text. The serial, not a nickname.
  std::string_view serial;
  // The GTE control register numbers. Beetle's gte.c is the authority for the meaning; the number
  // is stated once here so the reads in the owner are readable.
  std::uint32_t centreXRegister = 24;        // CR[24] OFX, signed 16.16
  std::uint32_t centreYRegister = 25;        // CR[25] OFY, signed 16.16
  std::uint32_t screenDistanceRegister = 26; // CR[26] H, projection-plane distance

  // The three publication entries, each one guest code reaches by `jal`.
  std::uint32_t projectionInit = 0; // the once-per-boot writer of H, OFX and OFY
  std::uint32_t setGeomOffset = 0;  // the per-frame centre publisher; the widening's latch site
  std::uint32_t setGeomScreen = 0;  // the H publisher; asserted, never changed

  // The retail 4:3 baseline the init publication must produce. These are read back out of the
  // guest's own coprocessor registers and compared against this, so a changed title is a refusal
  // rather than a silently different picture.
  std::int32_t retailScreenDistance = 0;
  std::int32_t retailCentreX = 0;
  std::int32_t retailCentreY = 0;

  // Call sites of `setGeomOffset` that must NOT be widened, because their `$a0` provably comes out
  // of the coprocessor register this owner moves. Empty for a title with no read-back. A title that
  // has one and lists nothing here would compound the margin, which is why the owner treats an
  // unknown call site as a refusal rather than defaulting to widening it.
  const std::uint32_t *passThroughCallSites = nullptr;
  std::size_t passThroughCallSiteCount = 0;

  // The call sites of `setGeom_offset` this owner has been told about, so an unexpected one is a
  // named refusal instead of a silent default.
  const std::uint32_t *centreCallSites = nullptr;
  std::size_t centreCallSiteCount = 0;
};

// Process-lifetime policy AND the publication owner. It answers which aspect the player selected and
// applies the matching plan to the title's own guest projection; a declaration alone cannot widen a
// frame (`external/psxport/docs/presentation-contract.md`, "Title-owned guest widescreen"). The plan
// itself is per-Game in the framework's own latch, so this holds no per-frame state of its own.
//
// Not `final`: each title derives a named owner from it, so the per-title type a test and a runtime
// reach for is a real type rather than a cast back to this one.
class GuestProjectionPublication : public GuestWidescreenProjection {
public:
  // The framework's own latch, injected so a hermetic test drives the production path.
  using Latch = GuestProjectionPlan (*)(Core *, GuestProjectionGeometry);
  // An authenticated original guest body, executed through Lightrec. Injected so a test can observe
  // the transformation without a guest image.
  using RetailBody = void (*)(Core &);

  GuestProjectionPublication(const ProjectionTitleFacts &facts, Latch latch);

  PresentationAspect presentationAspect(const Core &core) const override;

  // --- the latch site: set_geom_offset, reached per frame by the title's own view publication ------
  // The widened centre is `retail + margin`, in the title's own units, where `retail` is the
  // argument register as it stands RIGHT NOW — never a remembered value and never the register this
  // owner widened. That is what makes the widening idempotent by construction instead of by a guard
  // against accumulation, and it holds only for a call site whose argument does not already carry
  // the margin, which is what the pass-through list above names.
  void publishCentre(Core &core, const RetailBody &retail);

  // --- the once-per-boot writer: the projection init, which publishes the retail tuple from $zero --
  // This body writes the registers inline, so it republishes the retail baseline without passing
  // through the leaf. Reading the tuple back out of the coprocessor is what makes the retail 4:3
  // baseline a MEASURED fact instead of a constant this file happens to know. The widening itself
  // rides the per-frame leaf, and this site carries the same rule so a run whose per-frame
  // publication has not run yet is not left at a stale centre.
  void publishInitProjection(Core &core, const RetailBody &retail);

  // --- the H publisher: asserted, never modified -------------------------------------------------
  // H is the scale and a widening holds it fixed, and in these titles it is also the GTE near plane
  // and a HUD rectangle scalar. Overriding this site is what turns "the owner never touches H" from
  // an intention into a checked fact: the retail body runs on the title's own `$a0` and the value it
  // published is recorded.
  void observeScreenDistance(Core &core, const RetailBody &retail);

  const GuestProjectionPlan &plan() const {
    return plan_;
  }

  // The H the guest last published, from `$a0` at the measured leaf. Zero until it has run.
  std::int32_t publishedScreenDistance() const {
    return publishedScreenDistance_;
  }

  // True once a centre has been published from the guest's own argument register.
  bool published() const {
    return published_;
  }

  // How many centres this owner has published, and how many it deliberately passed through because
  // the argument came out of the coprocessor. A run reports both: a pass-through count of zero on a
  // title that declares a pass-through site means the site's path never ran, not that the policy
  // was never needed.
  std::size_t publications() const {
    return publications_;
  }
  std::size_t passThroughs() const {
    return passThroughs_;
  }

  // The centre the guest itself published, as observed. The base of the widening is the argument
  // register at the measured leaf and NEVER a remembered value: the leaf is a pure function of its
  // arguments, the coprocessor register is never fed back into `$a0` at a widening call site, and the
  // per-frame publication passes a live global — so yesterday's value is not retail, it is history.
  struct RetailCentre {
    std::int32_t x = 0;
    std::int32_t y = 0;
  };

  const RetailCentre &retailCentre() const {
    return retail_;
  }

  // Does this call site PASS THROUGH - that is, is it on the measured list whose argument arrives out
  // of the coprocessor register this owner moves? A site that is not on the list is a widening site.
  //
  // The predicate is named for the list it consults, not for the opposite: asking "does this widen?"
  // about a site the title never measured has no answer, and a function that answered "yes" would be
  // the exact way a future reader widened an unmeasured reach. `knowsCallSite` is the other half, and
  // `publishCentre` refuses rather than calling this one for a site it does not know.
  bool passesThrough(std::uint32_t callSite) const;

  // Does the owner's facts name this call site at all? This is the predicate `publishCentre` refuses
  // on, exposed so a test can pin the refusal without aborting the test process.
  bool knowsCallSite(std::uint32_t callSite) const;

  // Does the owner's facts declare a call site at all? A title with no pass-through list widens
  // every call site it was told about.
  bool declaresPassThrough() const {
    return facts_.passThroughCallSites != nullptr && facts_.passThroughCallSiteCount != 0;
  }

  const ProjectionTitleFacts &facts() const {
    return facts_;
  }

  // This title's one horizontal extent. Measured, not assumed: the guest's draw AREA is the PSX
  // default whole-display area, so its clip width and its projection width are the same number, and
  // that number is the display extent the title itself published through GP1 0xC0. The live value is
  // read from the framework's own decode of that command rather than written here, and refuses rather
  // than defaulting. A member rather than a static because a refusal has to name the title it refused
  // for, and a static could only say "some Crash title".
  GuestProjectionGeometry measuredGeometry(int displayWidth, int displayHeight) const;

  // Is this a guest RAM address the owner may read? Static and pure, because it is a fact about PSX
  // memory and not about any title - and because a bound expressed as a hex literal is exactly the
  // kind of thing a fixture using only low addresses cannot catch.
  static bool isGuestRam(std::uint32_t address);

  // The plan's centre for a given retail centre and margin. Pure in behaviour - no state, no latch -
  // but a member so the refusal it can raise names the title whose frame would have been wrong.
  std::int32_t widenedCentreX(std::int32_t retailCentreX, int margin) const;

private:
  GuestProjectionPlan relatch(Core &core);
  // The call site this override was reached from, as `$r31 - 4`, and whether it is one the facts name.
  std::uint32_t observedCallSite(const Core &core, const char *site) const;

  const ProjectionTitleFacts &facts_;
  Latch latch_;
  GuestProjectionPlan plan_;
  RetailCentre retail_;
  std::int32_t publishedScreenDistance_{};
  std::size_t publications_{};
  std::size_t passThroughs_{};
  bool published_{};
};

} // namespace crash
