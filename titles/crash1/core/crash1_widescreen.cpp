#include "crash1_widescreen.h"

#include "core.h"
#include "dynarec_dispatch.h"

#include <cstdlib>
#include <lucent/log.h>

namespace crash1 {
namespace {

void originalCentre(Core &core) {
  crash::dynarec::requireGuestReturn(crash::dynarec::callOriginal(core, kSetGeomOffset),
                                     "crash1-wide::centre original");
}

void originalInitProjection(Core &core) {
  crash::dynarec::requireGuestReturn(crash::dynarec::callOriginal(core, kProjectionInit),
                                     "crash1-wide::init projection original");
}

void originalScreenDistance(Core &core) {
  crash::dynarec::requireGuestReturn(crash::dynarec::callOriginal(core, kSetGeomScreen),
                                     "crash1-wide::screen distance original");
}

void centreOverride(Core *core) {
  Crash1Widescreen::from(*core).publishCentre(*core, originalCentre);
}

void initProjectionOverride(Core *core) {
  Crash1Widescreen::from(*core).publishInitProjection(*core, originalInitProjection);
}

void screenDistanceOverride(Core *core) {
  Crash1Widescreen::from(*core).observeScreenDistance(*core, originalScreenDistance);
}

} // namespace

const crash::ProjectionTitleFacts &Crash1Widescreen::facts() {
  // No pass-through list, because a title with no `cfc2` read of CR[24]/CR[25] anywhere in its image
  // has nothing whose argument can come out of the register this owner moves. Both lists are empty
  // because the leaf is reached indirectly, which is a measurement, not a gap.
  static const crash::ProjectionTitleFacts measured{
      .serial = "crash1-wide",
      .centreXRegister = 24,
      .centreYRegister = 25,
      .screenDistanceRegister = 26,
      .projectionInit = kProjectionInit,
      .setGeomOffset = kSetGeomOffset,
      .setGeomScreen = kSetGeomScreen,
      .retailScreenDistance = kRetailScreenDistance,
      .retailCentreX = kRetailCentreX,
      .retailCentreY = kRetailCentreY,
      .centreReach = kCentreReach,
      .passThroughCallSites = nullptr,
      .passThroughCallSiteCount = 0,
      .centreCallSites = nullptr,
      .centreCallSiteCount = 0,
  };
  return measured;
}

Crash1Widescreen &Crash1Widescreen::from(Core &core) {
  // The framework reaches the policy as a const base pointer, so the per-title state behind it comes
  // back through this checked downcast. A foreign result is a wiring defect that stops the run rather
  // than quietly presenting a 4:3 picture under a wide claim.
  auto *const owner = dynamic_cast<Crash1Widescreen *>(
      const_cast<GuestWidescreenProjection *>(core.runtime->guestWidescreenProjection()));
  if (!owner) {
    lucent::error("crash1-wide", "SCUS-949.00 guest widescreen override reached another title's policy");
    std::abort();
  }
  return *owner;
}

void installCrash1Widescreen(Core &core) {
  const struct Binding {
    std::uint32_t address;
    const char *owner;
    psx::cpu::NativeFunction function;
  } bindings[]{
      {kSetGeomOffset, "Crash SetGeomOffset", centreOverride},
      {kProjectionInit, "Crash GTE projection init", initProjectionOverride},
      {kSetGeomScreen, "Crash SetGeomScreen", screenDistanceOverride},
  };
  for (const Binding &binding : bindings) {
    if (!crash::dynarec::installOverride(core, binding.address, binding.owner, binding.function)) {
      std::abort();
    }
  }
  lucent::info("crash1-wide",
               "guest widescreen installed: SetGeomOffset 0x{:08X}, projection init 0x{:08X}, "
               "SetGeomScreen 0x{:08X}; retail centre {} {} and H {}",
               kSetGeomOffset,
               kProjectionInit,
               kSetGeomScreen,
               kRetailCentreX,
               kRetailCentreY,
               kRetailScreenDistance);
}

} // namespace crash1