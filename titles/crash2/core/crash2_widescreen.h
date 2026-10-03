// Crash Bandicoot 2 (SCUS-941.54) guest widescreen: this title's measured projection facts.
//
// The widening RULE is the shared `crash::GuestProjectionPublication` in `game/core/`: Crash 2
// publishes through the same three guest leaves as Crash 1 and Crash 3, measured byte-identical in
// `titles/crash2/executable.json`. This file carries only what is Crash 2's: the addresses, the
// measured call sites, the retail baseline, and the install.
//
// The projection is the GTE screen offset, not an authored triple and not a viewport rectangle: OFX
// is the pixel centre and H is the scale, so the widening moves OFX and holds OFY and H. H is held
// here on measured grounds rather than taste. Crash 2 keeps the projection-plane distance in a
// main-RAM global, 0x80060884, with four writers and seven readers, and TWO of those readers turn it
// into a gameplay-visible decision:
//
//   FUN_8003d3cc [0x8003D3CC,0x8003DA18) rejects a vertex when NOT (H < Z < 12000), so H is the GTE
//   near plane, exactly as in Crash 1.
//   FUN_8001645c [0x8001645C,0x80016A68) sizes and positions a 2D overlay rectangle from
//   `(H * fog * 0xAA >> 20) - 0x6C` clamped to 0..216, so H is also a HUD scalar here.
//
// Raising H would cull near geometry AND resize a 2D overlay. Moving OFX does neither, because a
// whole-image census finds no `cfc2` control read of CR[24] and none of CR[25]: no guest code in this
// title reads the published centre back, so nothing the owner moves can reach a decision.
#pragma once

#include "guest_projection_publication.h"

#include <cstdint>

class Core;

namespace crash2 {

// The three measured publication entries, each one guest code reaches by `jal`. A resident-word scan
// of all 81,408 words
// finds zero pointer-table entries equal to any of them, so no indirect reach exists.
inline constexpr std::uint32_t kProjectionInit = 0x8004EC30u; // publishes H, OFX and OFY once
inline constexpr std::uint32_t kSetGeomOffset = 0x8004EFE8u;  // publishes OFX and OFY; the latch site
inline constexpr std::uint32_t kSetGeomScreen = 0x8004F008u;  // publishes H; asserted, never changed

// The measured call sites of set_geom_offset, recovered by the owner as `$r31 - 4`. The owner refuses
// a call site that is not in this list rather than widening a reach this repository never measured.
inline constexpr std::uint32_t kCentreCallSites[] = {
    0x800179CCu, // the camera setup FUN_8001798c, which publishes the retail centre 0 and H 288
    0x80017F70u, // every frame, from FUN_80017bc4, with a live camera global as the centre
};

// The size, named once so the owner, its install log and its falsifiers cannot disagree about how many
// sites there are. This title has NO pass-through list: the census finds zero `cfc2` control reads of
// CR[24] and CR[25] anywhere in its 81,408 words, so nothing can hand this owner back a value that
// already carries the margin. The empty list is a measurement, and `declarePassThrough()` is the
// predicate that states it.
inline constexpr std::size_t kCentreCallSiteCount = sizeof(kCentreCallSites) / sizeof(kCentreCallSites[0]);

// The retail 4:3 baseline, read back out of the guest's own coprocessor registers and compared
// against these. Not constants this file happens to know: values the image was measured to publish.
inline constexpr std::int32_t kRetailScreenDistance = 1000; // 0x3E8, CR[26]
inline constexpr std::int32_t kRetailCentreX = 0;           // CR[24]
inline constexpr std::int32_t kRetailCentreY = 0;           // CR[25]

// The main-RAM global this title keeps the projection-plane distance in, named here for WHAT IT IS
// rather than for its role in a cull: four writers, seven readers, and two of those readers turn it
// into a gameplay decision. It is loader-created zero-initialised memory outside the executable's own
// text, which is why the census below cannot be a scan for a 4:3-width literal. This owner does not
// read or write it - the widening holds H through the coprocessor instead - but its address is the
// reason H is held, so it is recorded here and diffed against the manifest by the probe.
inline constexpr std::uint32_t kScreenDistanceCache = 0x80060884u;

// This title's owner. The rule is shared; the facts above are its.
class Crash2Widescreen final : public crash::GuestProjectionPublication {
public:
  using GuestProjectionPublication::GuestProjectionPublication;

  // This title's measured facts, from the constants above. The single place the two representations
  // are joined, so nothing else in this repository restates them.
  static const crash::ProjectionTitleFacts &facts();

  // This title's owner, reached from a Core that is running it. The checked downcast lives here so
  // no other file repeats the rule "the policy the runtime returns is the owner that published the
  // picture", and so a Core running another title's policy is a named refusal instead of a silent
  // no-op. Same idiom as Crash 1's owner.
  static Crash2Widescreen &from(Core &core);
};

// Install this title's three measured projection sites on one Core. Not reachable through
// PlatformHle, which covers the stock library services; the title owns its own geometry leaves.
void installCrash2Widescreen(Core &core);

} // namespace crash2
