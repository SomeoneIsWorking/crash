#include "crash2_widescreen.h"

namespace crash2 {

namespace {

// No pass-through list, because a title with no `cfc2` read of CR[24]/CR[25] anywhere in its image
// has nothing whose argument can come out of the register this owner moves. The list is a
// measurement that came back empty, and an empty one is stated rather than faked.
constexpr crash::ProjectionTitleFacts kMeasuredFacts{
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

} // namespace

const crash::ProjectionTitleFacts &Crash2Widescreen::facts() {
  return kMeasuredFacts;
}

} // namespace crash2
