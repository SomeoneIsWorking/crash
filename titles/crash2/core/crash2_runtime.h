#pragma once

#include "boundary_runtime.h"

namespace crash2 {

// Process-lifetime owner of Crash 2's measured executable facts. Native boot remains unavailable
// until the independent oracle validates and resumes the first syscall exception.
class Crash2Runtime final : public crash::BoundaryRuntime {
public:
  Crash2Runtime();

  const GuestProgramImage *guestProgramImage() const override;
  bool guestVramIsPicture(const Game &game) const override;

private:
  static const GuestProgramImage programImage_;
};

} // namespace crash2
