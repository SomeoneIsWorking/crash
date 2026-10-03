// Crash Bandicoot 1 (SCUS-949.00) guest widescreen: this title's measured projection facts.
//
// Crash 1 publishes its projection through the same three guest leaves as Crash 2 and Crash 3, and
// the leaves are measured to be the same code, so the widening RULE is the shared
// `crash::GuestProjectionPublication` in `game/core/`. This file carries only what is Crash 1's: the
// addresses, the retail 4:3 baseline, the measured call sites, and the install. The authority for all
// of them is `titles/crash1/executable.json` under `runtime.projection`.
//
// What the projection IS: the GTE screen offset, not a viewport rectangle. Beetle's `gte.c` names
// CR[24]=OFX, CR[25]=OFY, CR[26]=H and computes `SX = OFX + H*IR1/SZ`, so OFX is the pixel centre
// and H is the scale. A widening moves OFX and holds OFY and H; substituting a smaller H would be a
// zoom. A whole-image census finds no `cfc2` reader of any of the three control registers, so moving
// OFX cannot flip a guest decision, and this title has no horizontal cull to clip the revealed
// geometry - `crash1_horizontal_bound.*` owns the variable bound the guest does use and checks that a
// widening left it alone.
#pragma once

#include "guest_projection_publication.h"

#include <cstdint>

class Core;

namespace crash1 {

// The three measured publication entries, named by the guest leaf that reaches them.
inline constexpr std::uint32_t kProjectionInit = 0x80042B1Cu; // writes OFX, OFY and H once, from $zero
inline constexpr std::uint32_t kSetGeomOffset = 0x80042F8Cu;  // writes OFX and OFY; the widening's site
inline constexpr std::uint32_t kSetGeomScreen = 0x80042FACu;  // writes H; asserted, never changed

// HOW THE GUEST REACHES THE LEAF, measured rather than assumed. Decoding all 72,192 instruction
// words of the authenticated executable finds NO `jal` whose target is set_geom_offset, and the
// leaf's address appears as no resident word in either byte order, so the leaf is reached
// INDIRECTLY and `$r31` at the leaf carries the enclosing chain's return address rather than a call
// site. A live disc-backed run agrees: the owner is entered with `$r31 = 0x80017844`, in the camera
// setup FUN_80017790 and four past anything the shared rule would recognise as a site (docs/issues/
// 0025). Widening every reach is sound here BECAUSE this image has no `cfc2` control read of CR[24] or
// CR[25] at all, so no argument can arrive already carrying the margin.
inline constexpr crash::CentreReach kCentreReach = crash::CentreReach::IndirectCall;

// The retail 4:3 baseline this title's gte_init publishes, read back out of the guest's own
// coprocessor registers and compared against these rather than assumed.
inline constexpr std::int32_t kRetailScreenDistance = 1000; // 0x3E8, CR[26]
inline constexpr std::int32_t kRetailCentreX = 0;           // CR[24]
inline constexpr std::int32_t kRetailCentreY = 0;           // CR[25]

// This title's owner over the shared rule: the facts above, and nothing else. Crash 1 declares NO
// pass-through call site, because its image has no `cfc2` control read of CR[24]/CR[25] at all and
// therefore nothing that can hand the owner back a value already carrying the margin.
class Crash1Widescreen final : public crash::GuestProjectionPublication {
public:
  using GuestProjectionPublication::GuestProjectionPublication;

  // This title's measured facts. The single place the two representations are joined.
  static const crash::ProjectionTitleFacts &facts();

  // This title's owner, reached from a Core that is running it. The checked downcast lives here so
  // no other file repeats the rule "the policy the runtime returns is the owner that published the
  // picture", and a foreign result is a named refusal instead of a silent no-op. Same idiom as
  // `Crash1FrameDriver::from`.
  static Crash1Widescreen &from(Core &core);
};

// Install this title's three measured projection sites on one Core. Not reachable through
// PlatformHle, which covers the stock library services; the title owns its own geometry leaves.
void installCrash1Widescreen(Core &core);

} // namespace crash1