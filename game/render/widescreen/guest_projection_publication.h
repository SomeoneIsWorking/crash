// The Crash trilogy's ONE guest-projection widening rule, over the measured facts each title needs.
//
// WHY IT IS IN `game/`: all three North American Crash titles publish their projection through the
// same three guest leaves, and the leaves are byte-identical where they can be compared (`9aa95b09…`
// and `d8c79a8c…` in each manifest). That is direct evidence about the binaries, not franchise
// lineage.
//
// THE RULE: the titles' projection is the GTE screen offset, not a viewport rectangle. Beetle's
// `gte.c` names CR[24]=OFX, CR[25]=OFY, CR[26]=H and computes `SX = OFX + H*IR1/SZ`, so OFX is the
// pixel centre and H is the scale. Moving OFX and holding OFY and H widens the frustum by the canvas
// ratio and leaves central scale and vertical FOV alone; substituting a smaller H is a zoom. H is
// held because in all three titles it is also the GTE near plane (`H < Z < 12000`) and, in Crash 2 and
// Crash 3, a 2D overlay rectangle's height. The margin itself is NOT computed here: it comes from the
// framework's own latch through `guest_projection_plan`.
//
// WHY A CALL-SITE POLICY EXISTS, the one place the three titles differ. The widening's base is the
// title's own `$a0` as it stands right now, so it is idempotent only while the value arriving at the
// leaf does not already carry the margin. Crash 1 and Crash 2 have no `cfc2` control read of
// CR[24]/CR[25] anywhere, so nothing can feed the register back. Crash 3 has one: `FUN_8001cd80`
// hands a value read out of both registers straight back to `set_geom_offset`. A title with a
// read-back names the call sites that must pass through unchanged, and the owner refuses any call
// site it was not told about rather than widening a reach whose argument provenance nobody measured.
//
// The call site is recovered from `$r31 - 4`, exact for a `jal`. Every measured call site of every
// measured leaf is a `jal`, and a resident-word scan of each image finds no pointer-table entry equal
// to any of the three entries, so no indirect reach can make `$r31` something the owner was not told.
#pragma once

#include "guest_widescreen_projection.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>

class Core;
class Game;

namespace crash {

// How the guest reaches `setGeomOffset`, which decides whether `$r31` names a call site at all.
//
// A title's call sites are recovered as `$r31 - 4`, and that recovery is exact only for a `jal`. A
// whole-image scan of all three North American Crash executables finds NO `jal` to the leaf and no
// resident word equal to its address: the leaf is called through the engine's function-pointer table,
// so at the leaf `$r31` is the enclosing call chain's return address, not a site. A title therefore
// states which recovery its own image supports, and the owner refuses a combination it cannot honour.
enum class CentreReach {
  ReturnAddressCallSites, // every reach is a `jal`; `$r31 - 4` names the measured call site
  IndirectCall,           // the leaf is called indirectly; every reach widens
};

// The measured per-title projection facts. Every member was read out of that title's authenticated
// executable and is recorded in `titles/<title>/executable.json` under `runtime.projection`.
struct ProjectionTitleFacts {
  // For the log line and the refusal text. The serial, not a nickname.
  std::string_view serial;
  // The GTE control register numbers. Beetle's gte.c is the authority for the meaning.
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

  // How this image reaches the centre leaf. See `CentreReach`.
  CentreReach centreReach = CentreReach::ReturnAddressCallSites;

  // Call sites of `setGeomOffset` that must NOT be widened, because their `$a0` provably comes out
  // of the coprocessor register this owner moves. Empty for a title with no read-back. A title that
  // reaches the leaf INDIRECTLY cannot honour this list, because `$r31` does not name a site, and its
  // facts refuse to declare one rather than carrying a list nothing can select from.
  const std::uint32_t *passThroughCallSites = nullptr;
  std::size_t passThroughCallSiteCount = 0;

  // The call sites of `setGeom_offset` this owner has been told about, so an unexpected one is a
  // named refusal instead of a silent default. Required for a `ReturnAddressCallSites` reach, and
  // unused for an `IndirectCall` one.
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
  // the transformation without a guest image, and a std::function so the production trampoline can
  // bind the owner's own measured facts to the original call it has to make.
  using RetailBody = std::function<void(Core &)>;

  GuestProjectionPublication(const ProjectionTitleFacts &facts, Latch latch);

  PresentationAspect presentationAspect(const Core &core) const override;

  // This title's owner, reached from a Core that is running it. The framework hands the policy back
  // as a const base pointer, so the per-title state behind it comes back through this checked
  // downcast; a null or foreign result is a wiring defect that stops the run rather than quietly
  // presenting a 4:3 picture under a wide claim. The one resolver every title's overrides use.
  static const GuestProjectionPublication &from(const Core &core, std::string_view site);
  static GuestProjectionPublication &from(Core &core, std::string_view site);

  // Install this title's three measured projection leaves on one Core, each with the retail original
  // it runs. Not reachable through PlatformHle, which covers the stock library services: the titles
  // own their own geometry leaves. One implementation for all three titles - the facts already name
  // every address, so a per-title copy could only ever differ in the log line.
  void installSites(Core &core);

  // --- the latch site: set_geom_offset, reached per frame by the title's own view publication ------
  // The widened centre is `retail + margin`, in the title's own units, where `retail` is the
  // argument register as it stands RIGHT NOW — never a remembered value — so the widening is
  // idempotent by construction, for every call site that does not already carry the margin.
  void publishCentre(Core &core, const RetailBody &retail);

  // --- the once-per-boot writer: the projection init, which publishes the retail tuple from $zero --
  // This body writes the registers inline, so reading the tuple back out of the coprocessor is what
  // makes the retail 4:3 baseline a measured fact rather than a constant this file knows.
  void publishInitProjection(Core &core, const RetailBody &retail);

  // --- the H publisher: asserted, never modified -------------------------------------------------
  // H is the scale and a widening holds it fixed, and in these titles it is also the GTE near plane
  // and a HUD rectangle scalar. Overriding this site makes "the owner never touches H" checked.
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
  // title that declares a pass-through site means the site's path never ran.
  std::size_t publications() const {
    return publications_;
  }
  std::size_t passThroughs() const {
    return passThroughs_;
  }

  // The centre the guest itself published, as observed. The base of the widening is the argument
  // register at the measured leaf and NEVER a remembered value: the leaf is a pure function of its
  // arguments and the per-frame publication passes a live global, so yesterday's value is history.
  struct RetailCentre {
    std::int32_t x = 0;
    std::int32_t y = 0;
  };

  const RetailCentre &retailCentre() const {
    return retail_;
  }

  // Does this call site PASS THROUGH, i.e. is it on the measured list whose argument arrives out of
  // the coprocessor register this owner moves? A site the title never measured is not on it. Only
  // meaningful for a `ReturnAddressCallSites` reach: an `IndirectCall` title cannot select from this
  // list, and its facts refuse to declare one.
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

  // This title's one horizontal extent. Measured, not written: the guest's draw AREA is the PSX
  // default whole-display area, so the one extent it publishes is the display extent it sent through
  // GP1 0xC0, which the framework decodes and this owner reads. A member rather than a static
  // because a refusal has to name the title it refused for.
  GuestProjectionGeometry measuredGeometry(int displayWidth, int displayHeight) const;

  // Is this a guest RAM address the owner may read? Static and pure, because it is a fact about PSX
  // memory rather than about any title.
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
