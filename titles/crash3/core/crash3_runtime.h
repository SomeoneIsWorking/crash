#pragma once

#include "boundary_runtime.h"

namespace crash3 {

// Process-lifetime owner of Crash 3's measured executable facts. Native boot remains unavailable
// until execution is compared beyond the first real crt0 call boundary.
class Crash3Runtime final : public crash::BoundaryRuntime {
public:
  Crash3Runtime();

  const GuestProgramImage *guestProgramImage() const override;

private:
  static const GuestProgramImage programImage_;
};

} // namespace crash3
