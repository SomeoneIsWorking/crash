#include "crash3_widescreen.h"

#include "core.h"
#include "dynarec_dispatch.h"
#include "game.h"

#include <cstdlib>
#include <lucent/log.h>

namespace crash3 {
namespace {

void originalCentre(Core &core) {
  crash::dynarec::requireGuestReturn(crash::dynarec::callOriginal(core, kSetGeomOffset),
                                     "crash3-wide::centre original");
}

void originalInitProjection(Core &core) {
  crash::dynarec::requireGuestReturn(crash::dynarec::callOriginal(core, kProjectionInit),
                                     "crash3-wide::init projection original");
}

void originalScreenDistance(Core &core) {
  crash::dynarec::requireGuestReturn(crash::dynarec::callOriginal(core, kSetGeomScreen),
                                     "crash3-wide::screen distance original");
}

void centreOverride(Core *core) {
  Crash3Widescreen::from(*core).publishCentre(*core, originalCentre);
}

void initProjectionOverride(Core *core) {
  Crash3Widescreen::from(*core).publishInitProjection(*core, originalInitProjection);
}

void screenDistanceOverride(Core *core) {
  Crash3Widescreen::from(*core).observeScreenDistance(*core, originalScreenDistance);
}

} // namespace

const crash::ProjectionTitleFacts &Crash3Widescreen::facts() {
  // The pass-through list is not a preference. This is the only title of the three with a `cfc2`
  // control read of CR[24]/CR[25] anywhere in its image, so it is the only one where an argument
  // reaching the leaf can already carry a previous widening.
  static const crash::ProjectionTitleFacts measured{
      .serial = "crash3-wide",
      .centreXRegister = 24,
      .centreYRegister = 25,
      .screenDistanceRegister = 26,
      .projectionInit = kProjectionInit,
      .setGeomOffset = kSetGeomOffset,
      .setGeomScreen = kSetGeomScreen,
      .retailScreenDistance = kRetailScreenDistance,
      .retailCentreX = kRetailCentreX,
      .retailCentreY = kRetailCentreY,
      .passThroughCallSites = kPassThroughCallSites,
      .passThroughCallSiteCount = kPassThroughCallSiteCount,
      .centreCallSites = kCentreCallSites,
      .centreCallSiteCount = kCentreCallSiteCount,
  };
  return measured;
}

Crash3Widescreen &Crash3Widescreen::from(Core &core) {
  if (!core.runtime) {
    lucent::error("crash3-wide", "SCUS-942.44 guest widescreen override ran without its title runtime");
    std::abort();
  }
  // The policy is reached as a const base pointer because that is the framework's seam, so the
  // per-Core state behind it comes back through a checked downcast. A null or foreign result is a
  // wiring defect and stops the run rather than quietly presenting a 4:3 picture under a wide claim.
  auto *const policy = dynamic_cast<Crash3Widescreen *>(
      const_cast<GuestWidescreenProjection *>(core.runtime->guestWidescreenProjection()));
  if (!policy) {
    lucent::error("crash3-wide", "SCUS-942.44 guest widescreen override reached another title's policy");
    std::abort();
  }
  return *policy;
}

void installCrash3Widescreen(Core &core) {
  const struct Binding {
    std::uint32_t address;
    const char *owner;
    psx::cpu::NativeFunction function;
  } bindings[]{
      {kSetGeomOffset, "Crash 3 SetGeomOffset", centreOverride},
      {kProjectionInit, "Crash 3 GTE projection init", initProjectionOverride},
      {kSetGeomScreen, "Crash 3 SetGeomScreen", screenDistanceOverride},
  };
  for (const Binding &binding : bindings) {
    if (!crash::dynarec::installOverride(core, binding.address, binding.owner, binding.function)) {
      std::abort();
    }
  }
  lucent::info("crash3-wide",
               "guest widescreen installed: SetGeomOffset 0x{:08X}, projection init 0x{:08X}, "
               "SetGeomScreen 0x{:08X}; retail centre {} {} and H {}; the horizontal centre widens at "
               "{} of {} measured call sites and passes through 0x{:08X}, whose argument is read back "
               "out of CR[24]",
               kSetGeomOffset,
               kProjectionInit,
               kSetGeomScreen,
               kRetailCentreX,
               kRetailCentreY,
               kRetailScreenDistance,
               kCentreCallSiteCount - kPassThroughCallSiteCount,
               kCentreCallSiteCount,
               kPassThroughCallSites[0]);
}
} // namespace crash3
