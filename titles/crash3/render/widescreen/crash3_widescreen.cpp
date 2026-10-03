#include "crash3_widescreen.h"

namespace crash3 {

namespace {

// The pass-through list is not a preference. This is the only title of the three with a `cfc2`
// control read of CR[24]/CR[25] anywhere in its image, so it is the only one where an argument
// reaching the leaf can already carry a previous widening.
constexpr crash::ProjectionTitleFacts kMeasuredFacts{
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

} // namespace

const crash::ProjectionTitleFacts &Crash3Widescreen::facts() {
  return kMeasuredFacts;
}

} // namespace crash3
