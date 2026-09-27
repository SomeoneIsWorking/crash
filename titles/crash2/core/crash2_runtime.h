#pragma once

#include "boundary_runtime.h"
#include "crash2_widescreen.h"
#include "gpu_vk.h"
#include "native_frame_loop_contract.h"

#include <memory>

namespace crash2 {

// Process-lifetime owner of Crash 2's measured executable facts. Native boot remains unavailable
// until the independent oracle validates and resumes the first syscall exception.
class Crash2Runtime final : public crash::BoundaryRuntime {
public:
  Crash2Runtime();

  const GuestProgramImage *guestProgramImage() const override;
  const PlatformHlePlan *platformHlePlan() const override;
  bool guestVramIsPicture(const Game &game) const override;
  std::unique_ptr<FrameDriver> createFrameDriver(Game &game) override;
  // The measured projection owner. This is the override `GameRuntime` left returning nullptr, which
  // is an absence rather than a capability: Crash 2 publishes its projection through three measured
  // guest leaves, and this runtime now hands the framework the owner behind them. The policy answers
  // which aspect the player selected; the owner publishes a matching guest projection
  // (external/psxport/docs/presentation-contract.md, "Title-owned guest widescreen").
  const GuestWidescreenProjection *guestWidescreenProjection() const override;

  const crash::NativeFrameLoopContract &nativeFrameLoopContract() const;

  // The measured projection owner this runtime hands the framework's latch. Exposed for the tests
  // that drive the publication sites, and for nothing else: the shipping path reaches it through
  // `guestWidescreenProjection()` like any other title policy.
  Crash2Widescreen &widescreen() {
    return widescreen_;
  }
  const Crash2Widescreen &widescreen() const {
    return widescreen_;
  }

private:
  static const GuestProgramImage programImage_;
  static const PlatformHlePlan platformPlan_;
  Crash2Widescreen widescreen_{Crash2Widescreen::facts(), &gpu_vk_latch_guest_projection};
};

} // namespace crash2
