// Crash Bandicoot 1 (SCUS-949.00) guest widescreen: this title's own projection owners.
//
// Every address and value below was read out of the authenticated executable by
// tools/probe_crash1_projection.py, which re-derives the whole census on demand and refuses when the
// image disagrees. The recorded ranges live in titles/crash1/executable.json under
// runtime.projection; nothing here is a tuned constant.
//
// WHAT CRASH 1'S PROJECTION IS. Not H/OFX/OFY-as-a-triple, and not a viewport rectangle: the GTE
// screen offset. Beetle's gte.c names CR[24]=OFX, CR[25]=OFY, CR[26]=H, and its RTPS computes
// `h_div_sz = Divide(H, Z_FIFO(3))` then `TransformXY(h_div_sz, ...)`, so a projected point lands at
//     SX = OFX + (H * IR1) / SZ
// with OFX the pixel centre and H the scale. A whole-image census of all 72192 instruction words
// finds exactly TWO control-register writers for each of OFX, OFY and H, and ZERO control-register
// readers of any of them:
//
//     0x80042B88  ctc2 $zero, 0xC000   OFX = 0        }  gte_init 0x80042B1C, called once from
//     0x80042B8C  ctc2 $zero, 0xC800   OFY = 0        }  0x80016558 inside Init 0x8001652C
//     0x80042B68  ctc2 $t0,   0xD000   H   = 0x3E8    }  (1000)
//
//     0x80042F94  ctc2 $a0, 0xC000     OFX = a0 << 16 }  set_geom_offset 0x80042F8C
//     0x80042F98  ctc2 $a1, 0xC800     OFY = a1 << 16 }
//     0x80042FAC  ctc2 $a0, 0xD000     H   = a0        }  set_geom_screen  0x80042FAC
//
// CRASH 1'S RETAIL 4:3 PROJECTION IS THEREFORE OFX = 0, OFY = 0, and a per-camera-mode H. The zero
// is not an omission: the title's own 3x3 view matrix is published into the GTE translation vector
// (CR[4..7], written through SetGeomTranslation 0x80042C2C and the 0x80042E9C/0x80042ECC/0x80042EFC
// matrix leaves), and that matrix already carries the screen centring. H is chosen from
// `*(int *)(DAT_8005C53C + 0x114)` in FUN_80017790: 0x25->500, 0x1E->960, 0x38->800, 0x3C->460,
// 0x5A->288, stored in the global DAT_800578D0 and republished per frame by 0x80026770.
//
// WHY MOVING OFX IS THE WIDENING HERE, IN ONE LINE. With SX = OFX + H*x/z, the world half-width
// visible at depth pz is (displayWidth/2 - OFX) * pz / H. Retail is displayWidth 320 with OFX 0, so
// the visible half-width is 160*pz/H. A 428-wide canvas holding that same 4:3 frame at its ORIGINAL
// pixel scale is the same equation with OFX = (428-320)/2 = 54 and H unchanged - which is exactly the
// rule in external/psxport/docs/presentation-contract.md, "What counts as a widening": the pair
// (centre, scale) widens the frustum by the canvas ratio and leaves central scale and vertical FOV
// untouched. Substituting a smaller H would be a ZOOM, and this owner never touches H at all.
//
// THE ZERO READERS ARE LOAD-BEARING. No guest branch tests OFX, OFY or H, so moving OFX cannot flip a
// decision the title makes. That is measured, not assumed: the census prints the reader count for
// each register and fails if it is ever non-zero. (The three `mfc2 $x,0xC000` sites at 0x800347B4,
// 0x80034CF0 and 0x80035194 look like OFX readers and are NOT: they are mfc2 DATA reads of GTE data
// register 24, a reserved slot, and the `beq $sp,$zero` after each is a test of a reserved register
// that always reads zero.)
#pragma once

#include "guest_widescreen_projection.h"

#include <cstdint>

class Core;

namespace crash1 {

// The two control-register writers of the horizontal centre, and the one writer of H, named by the
// instruction that writes them. Each range is in titles/crash1/executable.json.
inline constexpr std::uint32_t kProjectionInit = 0x80042B1Cu; // writes OFX, OFY and H once
inline constexpr std::uint32_t kSetGeomOffset = 0x80042F8Cu;  // writes OFX and OFY; the latch site
inline constexpr std::uint32_t kSetGeomScreen = 0x80042FACu;  // writes H; asserted, never changed

// The measured call sites. tools/probe_crash1_projection.py re-measures these from the image and
// fails when they move; a function-pointer reach is the null it cannot see, and it is stated.
inline constexpr std::uint32_t kProjectionInitCallSite = 0x80016558u;
inline constexpr std::uint32_t kSetGeomOffsetCallSiteA = 0x8001783Cu; // in the camera setup, FUN_80017790
inline constexpr std::uint32_t kSetGeomOffsetCallSiteB = 0x80017F00u; // per frame, in FUN_80017A14
inline constexpr std::uint32_t kSetGeomScreenCallSiteA = 0x80017830u; // in the camera setup
inline constexpr std::uint32_t kSetGeomScreenCallSiteB = 0x80026770u; // per frame

// The retail projection gte_init publishes, read out of the words at 0x80042B68/0x80042B88/0x80042B8C.
inline constexpr std::int32_t kRetailScreenDistance = 1000; // 0x3E8, CR[26]
inline constexpr std::int32_t kRetailCentreX = 0;           // CR[24]
inline constexpr std::int32_t kRetailCentreY = 0;           // CR[25]

// The one place the title writes the guest draw AREA (GP1 0xE1): FUN_80042A04 stores
// `(GPUSTAT & 0x3FFF) | 0xE1001000`, i.e. origin (0, 4) with width and height zero, which on a PSX
// IS the whole display area. Measured consequence: this title has no separate clip rectangle, so
// there is no second rectangle for a widening to move. The display extent it does publish comes from
// GP1 0xC0 through the GPU driver pointer table at 0x80054A24, whose entry 7 is FUN_80041C38.
inline constexpr std::uint32_t kGuestDrawAreaPublication = 0x80042A58u;
inline constexpr std::uint32_t kDisplayModePublication = 0x80041C38u;
inline constexpr std::uint32_t kGpuDriverTable = 0x80054A24u;
inline constexpr unsigned kDisplayModeTableIndex = 7u;

// Process-lifetime policy AND the publication owner. It answers which aspect the player selected and
// it applies the matching plan to the title's own guest projection; a declaration alone cannot widen
// a frame (external/psxport/docs/presentation-contract.md, "Title-owned guest widescreen"). The plan
// itself is per-Game in the framework's own latch, so this holds no per-frame state of its own.
class Crash1Widescreen final : public GuestWidescreenProjection {
public:
  // The framework's own latch, injected so a hermetic test drives the production path.
  using Latch = GuestProjectionPlan (*)(Core *, GuestProjectionGeometry);
  // An authenticated original guest body, executed through Lightrec. Injected so a test can observe
  // the transformation without a guest image.
  using RetailBody = void (*)(Core &);

  explicit Crash1Widescreen(Latch latch);

  PresentationAspect presentationAspect(const Core &core) const override;

  // --- site 1: set_geom_offset, 0x80042F8C — the latch site, and it runs EVERY FRAME ------------
  // ---------------------------------------------------------------------------------------------
  // `jal 0x80042F8C` appears at exactly two addresses, and the second (0x80017F00, inside the
  // per-frame view publication FUN_80017A14 which the core loop reaches at 0x800123BC) runs once per
  // frame. So the guest republishes its own centre every frame and this owner re-latches with it:
  // there is no per-frame host-side re-publish, no guest call at a frame boundary, and no direct
  // write to a coprocessor register. The widened centre is `retail + margin`, in the title's own
  // units, where `retail` is the argument register as it stands RIGHT NOW — never a remembered value
  // and never the register this owner widened — which is what makes the widening idempotent by
  // construction instead of by a guard against accumulation.
  void publishCentre(Core &core, const RetailBody &retail);

  // --- site 2: gte_init, 0x80042B1C — the once-per-boot writer of OFX = OFY = 0 -----------------
  // ---------------------------------------------------------------------------------------------
  // This body writes CR[24] and CR[25] from `$zero` inline, so it republishes the retail zero
  // without passing through the leaf. It runs once (0x80016558, inside Init 0x8001652C) and the
  // first per-frame publication follows it, but a run whose camera setup never runs would keep the
  // retail centre, so this site carries the same widening rather than being assumed redundant.
  void publishInitProjection(Core &core, const RetailBody &retail);

  // --- site 3: set_geom_screen, 0x80042FAC — asserted, never modified ---------------------------
  // ---------------------------------------------------------------------------------------------
  // H is the scale, and a widening holds it fixed. This override exists to make that a checked fact
  // rather than an intention: the retail body runs, and the H the guest published is recorded so a
  // run can report the per-camera-mode value it actually used.
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

  // The centre the guest itself published, as observed. The base of the widening is the argument
  // register at the measured leaf and NEVER a remembered value: the leaf is a pure function of its
  // arguments, the coprocessor register is never fed back into `$a0`, and 0x80017F00 passes a live
  // global (the camera-shake word DAT_8006193C) — so yesterday's value is not retail, it is history.
  struct RetailCentre {
    std::int32_t x = 0;
    std::int32_t y = 0;
  };

  const RetailCentre &retailCentre() const {
    return retail_;
  }

  // The one horizontal extent this title has. Measured, not assumed: the guest's draw area is the
  // PSX default whole-display-area, so its clip width and its projection width are the same number,
  // and that number is the display extent the title itself published through GP1 0xC0. The live
  // value is read from the framework's own decode of that command rather than written here, and
  // refuses rather than defaulting.
  static GuestProjectionGeometry measuredGeometry(int displayWidth, int displayHeight);

  // Is this a guest RAM address the owner may read? Pure and public because the bound is exactly
  // the kind of thing a fixture using only low addresses cannot catch.
  static bool isGuestRam(std::uint32_t address);

  // The plan's centre for a given retail centre and margin. Pure, so the tests can pin the rule and
  // this file keeps no second copy of the arithmetic.
  static std::int32_t widenedCentreX(std::int32_t retailCentreX, int margin);

  // This title's owner, reached from a Core that is running it. The checked downcast lives here so
  // no other file repeats the rule "the policy the runtime returns is the owner that published the
  // picture", and so a Core running another title's policy is a named refusal instead of a silent
  // no-op. Same idiom as `Crash1FrameDriver::from`.
  static Crash1Widescreen &from(Core &core);

private:
  GuestProjectionPlan relatch(Core &core);

  Latch latch_;
  GuestProjectionPlan plan_;
  RetailCentre retail_;
  std::int32_t publishedScreenDistance_{};
  bool published_{};
};

// Install this title's three measured projection sites on one Core. Not reachable through
// PlatformHle, which covers the stock library services; the title owns its own geometry leaves.
void installCrash1Widescreen(Core &core);

} // namespace crash1
