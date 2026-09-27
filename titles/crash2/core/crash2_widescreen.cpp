#include "crash2_widescreen.h"

#include "core.h"
#include "dynarec_dispatch.h"
#include "game.h"

#include <cstdlib>
#include <lucent/log.h>

namespace crash2 {
namespace {

void originalCentre(Core &core) {
  crash::dynarec::requireGuestReturn(crash::dynarec::callOriginal(core, kSetGeomOffset),
                                     "crash2-wide::centre original");
}

void originalInitProjection(Core &core) {
  crash::dynarec::requireGuestReturn(crash::dynarec::callOriginal(core, kProjectionInit),
                                     "crash2-wide::init projection original");
}

void originalScreenDistance(Core &core) {
  crash::dynarec::requireGuestReturn(crash::dynarec::callOriginal(core, kSetGeomScreen),
                                     "crash2-wide::screen distance original");
}

void centreOverride(Core *core) {
  Crash2Widescreen::from(*core).publishCentre(*core, originalCentre);
}

void initProjectionOverride(Core *core) {
  Crash2Widescreen::from(*core).publishInitProjection(*core, originalInitProjection);
}

void screenDistanceOverride(Core *core) {
  Crash2Widescreen::from(*core).observeScreenDistance(*core, originalScreenDistance);
}

} // namespace

const crash::ProjectionTitleFacts &Crash2Widescreen::facts() {
  // A title with no `cfc2` read of CR[24]/CR[25] anywhere in its image declares NO pass-through
  // call site, because there is nothing whose argument can come out of the register this owner moves.
  // The list is a measurement that came back empty, and an empty one is stated rather than faked.
  static const crash::ProjectionTitleFacts measured{
      .serial = "crash2-wide",
      .centreXRegister = 24,
      .centreYRegister = 25,
      .screenDistanceRegister = 26,
      .projectionInit = kProjectionInit,
      .setGeomOffset = kSetGeomOffset,
      .setGeomScreen = kSetGeomScreen,
      .retailScreenDistance = kRetailScreenDistance,
      .retailCentreX = kRetailCentreX,
      .retailCentreY = kRetailCentreY,
      .passThroughCallSites = nullptr,
      .passThroughCallSiteCount = 0,
      .centreCallSites = kCentreCallSites,
      .centreCallSiteCount = kCentreCallSiteCount,
  };
  return measured;
}

Crash2Widescreen &Crash2Widescreen::from(Core &core) {
  if (!core.runtime) {
    lucent::error("crash2-wide", "SCUS-941.54 guest widescreen override ran without its title runtime");
    std::abort();
  }
  // The policy is reached as a const base pointer because that is the framework's seam, so the
  // per-Core state behind it comes back through a checked downcast. A null or foreign result is a
  // wiring defect and stops the run rather than quietly presenting a 4:3 picture under a wide claim.
  auto *const policy = dynamic_cast<Crash2Widescreen *>(
      const_cast<GuestWidescreenProjection *>(core.runtime->guestWidescreenProjection()));
  if (!policy) {
    lucent::error("crash2-wide", "SCUS-941.54 guest widescreen override reached another title's policy");
    std::abort();
  }
  return *policy;
}

void installCrash2Widescreen(Core &core) {
  const struct Binding {
    std::uint32_t address;
    const char *owner;
    psx::cpu::NativeFunction function;
  } bindings[]{
      {kSetGeomOffset, "Crash 2 SetGeomOffset", centreOverride},
      {kProjectionInit, "Crash 2 GTE projection init", initProjectionOverride},
      {kSetGeomScreen, "Crash 2 SetGeomScreen", screenDistanceOverride},
  };
  for (const Binding &binding : bindings) {
    if (!crash::dynarec::installOverride(core, binding.address, binding.owner, binding.function)) {
      std::abort();
    }
  }
  lucent::info("crash2-wide",
               "guest widescreen installed: SetGeomOffset 0x{:08X}, projection init 0x{:08X}, "
               "SetGeomScreen 0x{:08X}; retail centre {} {} and H {}; the horizontal centre widens "
               "and H is held because it is this title's GTE near plane and a HUD rectangle scalar",
               kSetGeomOffset,
               kProjectionInit,
               kSetGeomScreen,
               kRetailCentreX,
               kRetailCentreY,
               kRetailScreenDistance);
}

} // namespace crash2
