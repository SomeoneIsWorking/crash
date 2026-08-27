#pragma once

#include "boundary_runtime.h"
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

  const crash::NativeFrameLoopContract &nativeFrameLoopContract() const;

private:
  static const GuestProgramImage programImage_;
  static const PlatformHlePlan platformPlan_;
};

} // namespace crash2
