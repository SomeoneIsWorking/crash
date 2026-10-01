// Crash Bandicoot 3 (SCUS-942.44) guest widescreen: this title's own projection owners.
//
// Every address and value below was read out of the authenticated executable. The recorded facts live
// in `titles/crash3/executable.json` under `runtime.projection`; nothing here is a tuned constant.
//
// WHAT CRASH 3'S PROJECTION IS, AND IT IS THE SAME SHAPE AS CRASH 1 AND CRASH 2. Beetle's `gte.c`
// names CR[24]=OFX, CR[25]=OFY, CR[26]=H, and its RTPS computes `h_div_sz = Divide(H, Z_FIFO(3))` then
// `TransformXY`, so a projected point lands at `SX = OFX + (H * IR1) / SZ`. A whole-image census of
// all 82,944 instruction words finds exactly TWO control-register writers for each of OFX, OFY and H:
//
//   0x8004F3C8  ctc2 $t0, 0xD000   H  = 0x3E8 (1000)  }  the projection init 0x8004F37C, called
//   0x8004F3E8  ctc2 $zero, 0xC000  OFX = 0           }  once from 0x800154D8, which also
//   0x8004F3EC  ctc2 $zero, 0xC800  OFY = 0           }  publishes ZSF3, ZSF4, DQA and DQB
//
//   0x8004F70C  ctc2 $a0, 0xC000   OFX = a0 << 16    }  set_geom_offset 0x8004F704, whose body is
//   0x8004F710  ctc2 $a1, 0xC800   OFY = a1 << 16    }  BYTE-IDENTICAL to Crash 2's 0x8004EFE8
//   0x8004F724  ctc2 $a0, 0xD000   H   = a0          }  set_geom_screen 0x8004F724 - same sha256
//
// WHY H IS HELD HERE, MEASURED RATHER THAN ASSUMED. Crash 3 keeps the projection-plane distance in a
// main-RAM global, `0x80065D54`, and TWO consumers turn it into a gameplay-visible decision:
//
//   FUN_8003c3d0 [0x8003C3D0,0x8003C994) rejects a vertex when NOT (H < Z < 12000), with the bound
//   passed in from FUN_8001c824 at 0x8001C880. H is the GTE NEAR PLANE, as in Crash 1 and Crash 2.
//   FUN_80016634 [0x80016634,0x80016CE8) sizes and positions a 2D overlay rectangle from
//   `(H * fog * 0xAA >> 20) - 0x6C` and clamps it to 0..216, so H is a HUD scalar here too.
//
// Raising H would cull near geometry AND resize a 2D overlay, so this owner never touches it.
//
// AND WHY THIS TITLE NEEDS A CALL-SITE POLICY, WHICH CRASH 1 AND CRASH 2 DO NOT. The widening's base
// is the title's own `$a0` as it stands right now, so it is idempotent ONLY while the value arriving at
// the leaf does not already carry the margin. Crash 3 is the one title of the three that reads the
// published centre back out of the coprocessor:
//
//   0x8004F6E4  cfc2 $t0, $24   CR[24] OFX      }  FUN_8004f6e4 [0x8004F6E4,0x8004F704), called
//   0x8004F6E8  cfc2 $t1, $25   CR[25] OFY      }  ONCE, from 0x8001CF8C
//   0x8004F6EC  sra  $t0, $t0, 16               }
//   0x8004F6F0  sra  $t1, $t1, 16               }
//   0x8004F6F4  sw   $t0, 0($a0)                }
//   0x8004F6F8  sw   $t1, 0($a1)                }
//
// and `FUN_8001cd80` hands the result straight back to `set_geom_offset` at 0x8001D09C. Widening THAT
// call site would add the margin again on every pass of that path, so it is a measured pass-through:
// the argument arrives from the register this owner moves, so the owner must republish it unchanged.
//
// The other three call sites widen, and each one's argument provably does not come from the
// coprocessor: 0x8001892C passes the constant 0 (the camera setup), 0x80018C04 passes a guest global
// (the per-frame publication, every frame), and 0x8001CFC4 passes a sign-corrected halving of a table
// word. The owner refuses any call site that is not in its measured list, so a reach this repository
// never measured cannot be widened by default.
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

// This title's owner. The rule is shared; the facts above are its, and the pass-through list is the
// one place this title differs from its siblings.
class Crash3Widescreen final : public crash::GuestProjectionPublication {
public:
  using GuestProjectionPublication::GuestProjectionPublication;

  // This title's measured facts, from the constants above. The single place the two representations
  // are joined, so nothing else in this repository restates them.
  static const crash::ProjectionTitleFacts &facts();

  // This title's owner, reached from a Core that is running it. The checked downcast lives here so
  // no other file repeats the rule "the policy the runtime returns is the owner that published the
  // picture", and so a Core running another title's policy is a named refusal instead of a silent
  // no-op. Same idiom as Crash 1's owner.
  static Crash3Widescreen &from(Core &core);
};

// Install this title's three measured projection sites on one Core. Not reachable through
// PlatformHle, which covers the stock library services; the title owns its own geometry leaves.
void installCrash3Widescreen(Core &core);

} // namespace crash3
