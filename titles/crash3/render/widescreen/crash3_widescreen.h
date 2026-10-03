// Crash Bandicoot 3 (SCUS-942.44) guest widescreen: this title's measured projection facts.
//
// The widening RULE is the shared `crash::GuestProjectionPublication` in `game/core/`; this file
// carries only what is Crash 3's: the addresses, the measured call sites, the pass-through site, the
// retail baseline and the install. `titles/crash3/executable.json` under `runtime.projection` is the
// authority.
//
// The projection is the GTE screen offset, the same shape as Crash 1 and Crash 2, and a whole-image
// census of all 82,944 instruction words finds exactly TWO control-register writers for each of OFX,
// OFY and H: the projection init 0x8004F37C (H 0x3E8, OFX 0, OFY 0) and the leaves set_geom_offset
// 0x8004F704 and set_geom_screen 0x8004F724, whose bodies are byte-identical to Crash 2's.
//
// H is held on measured grounds: Crash 3 keeps the projection-plane distance in a main-RAM global,
// 0x80065D54, and TWO consumers turn it into a gameplay-visible decision.
//
//   FUN_8003c3d0 [0x8003C3D0,0x8003C994) rejects a vertex when NOT (H < Z < 12000), with the bound
//   passed in from FUN_8001c824 at 0x8001C880. H is the GTE near plane, as in the other two titles.
//   FUN_80016634 [0x80016634,0x80016CE8) sizes and positions a 2D overlay rectangle from
//   `(H * fog * 0xAA >> 20) - 0x6C` clamped to 0..216, so H is a HUD scalar here too.
//
// Raising H would cull near geometry AND resize a 2D overlay, so this owner never touches it.
//
// WHY THIS TITLE NEEDS A CALL-SITE POLICY, WHICH THE OTHER TWO DO NOT. The widening's base is the
// title's own `$a0` as it stands right now, so it is idempotent only while the value arriving at the
// leaf does not already carry the margin. Crash 3 is the one title of the three that reads the
// published centre back out of the coprocessor: FUN_8004f6e4 [0x8004F6E4,0x8004F704) `cfc2`s CR[24]
// and CR[25], and FUN_8001cd80 hands the result straight back to `set_geom_offset` at 0x8001D09C.
// Widening THAT call site would add the margin again on every pass, so it is a measured pass-through:
// the argument arrives from the register this owner moves, so the owner republishes it unchanged.
//
// The other three call sites widen, and each one's argument provably does not come from the
// coprocessor: 0x8001892C passes the constant 0 (the camera setup), 0x80018C04 passes a guest global
// (the per-frame publication), and 0x8001CFC4 passes a sign-corrected halving of a table word. The
// owner refuses any call site that is not in its measured list, so an unmeasured reach cannot be
// widened by default.
#pragma once

#include "guest_projection_publication.h"

#include <cstdint>

class Core;

namespace crash3 {

// The three measured publication entries, each one guest code reaches by `jal`. A resident-word scan
// of all 82,944 words
// finds zero pointer-table entries equal to any of them, so no indirect reach exists.
inline constexpr std::uint32_t kProjectionInit = 0x8004F37Cu; // publishes H, OFX and OFY once
inline constexpr std::uint32_t kSetGeomOffset = 0x8004F704u;  // publishes OFX and OFY; the latch site
inline constexpr std::uint32_t kSetGeomScreen = 0x8004F724u;  // publishes H; asserted, never changed

// The measured call sites of set_geom_offset, recovered by the owner as `$r31 - 4`. The owner refuses
// a call site that is not in this list rather than widening a reach this repository never measured.
inline constexpr std::uint32_t kCentreCallSites[] = {
    0x8001892Cu, // the camera setup FUN_800188ec, which publishes the retail centre 0 and H 288
    0x80018C04u, // every frame, from FUN_80018a54, with a live camera global as the centre
    0x8001CFC4u, // a transition path, with a sign-corrected halving of a table word as the centre
    0x8001D09Cu, // a transition path, with the CENTRE READ BACK OUT OF CR[24]/CR[25]
};

// The one call site that must NOT be widened, and why. Its `$a0` is `lw $a0, 184($sp)` at 0x8001D094,
// and that stack slot was written by the `jal 0x8004F6E4` at 0x8001CF8C - that is, from the very
// register this owner moves. Widening it would compound.
inline constexpr std::uint32_t kPassThroughCallSites[] = {0x8001D09Cu};

// The sizes, named once so the owner, its install log and its falsifiers cannot disagree about how
// many sites there are or how many of them pass through.
inline constexpr std::size_t kCentreCallSiteCount = sizeof(kCentreCallSites) / sizeof(kCentreCallSites[0]);
inline constexpr std::size_t kPassThroughCallSiteCount =
    sizeof(kPassThroughCallSites) / sizeof(kPassThroughCallSites[0]);
static_assert(kPassThroughCallSiteCount < kCentreCallSiteCount,
              "a title cannot pass through every centre call site and still widen anything");

// The retail 4:3 baseline, read back out of the guest's own coprocessor registers and compared
// against these. Not constants this file happens to know: values the image was measured to publish.
inline constexpr std::int32_t kRetailScreenDistance = 1000; // 0x3E8, CR[26]
inline constexpr std::int32_t kRetailCentreX = 0;           // CR[24]
inline constexpr std::int32_t kRetailCentreY = 0;           // CR[25]

// The main-RAM global this title keeps the projection-plane distance in, named here for WHAT IT IS
// rather than for its role in a cull. Two of its readers turn it into a gameplay decision - the
// near-plane reject and the HUD rectangle - so it is the reason H is held. This owner does not read
// or write it: the widening holds H through the coprocessor instead. Note that BOTH its writers are
// pointer-form (`sw $v, 0xC4($base)`), which a lui+displacement census structurally cannot see, so
// the manifest names them and the probe verifies their instruction words.
inline constexpr std::uint32_t kScreenDistanceCache = 0x80065D54u;

// This title's owner. The rule is shared; the facts above are its, including the pass-through list.
// The lookup, the install and the three leaves are `crash::GuestProjectionPublication`'s, one
// implementation for all three titles.
class Crash3Widescreen final : public crash::GuestProjectionPublication {
public:
  using GuestProjectionPublication::GuestProjectionPublication;

  // This title's measured facts. The single place the two representations are joined.
  static const crash::ProjectionTitleFacts &facts();
};

} // namespace crash3
