#include "crash1_widescreen.h"

namespace crash1 {

namespace {

// No pass-through list, because a title with no `cfc2` read of CR[24]/CR[25] anywhere in its image
// has nothing whose argument can come out of the register this owner moves. Both call-site lists
// are empty because the leaf is reached indirectly, which is a measurement, not a gap.
constexpr crash::ProjectionTitleFacts kMeasuredFacts{
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

} // namespace

const crash::ProjectionTitleFacts &Crash1Widescreen::facts() {
  return kMeasuredFacts;
}

} // namespace crash1
