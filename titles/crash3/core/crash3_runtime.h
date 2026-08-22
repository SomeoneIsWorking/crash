#pragma once

#include "boundary_runtime.h"

namespace crash3 {

// Process-lifetime owner of SCUS-94244's measured executable facts. Native boot remains unavailable
// until the independent oracle can resume beyond the measured EnterCriticalSection syscall.
class Crash3Runtime final : public crash::BoundaryRuntime {
public:
  Crash3Runtime();

  const GuestProgramImage *guestProgramImage() const override;

private:
  static const GuestProgramImage programImage_;
};

} // namespace crash3
