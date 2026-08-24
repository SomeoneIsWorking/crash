#pragma once

#include "boundary_runtime.h"

namespace crash1 {

// Process-lifetime owner of Crash 1's framework-facing behavior. The port currently reaches only a
// generated CPU boundary, so there are no game contexts, host overrides, or runtime products yet.
class Crash1Runtime final : public crash::BoundaryRuntime {
public:
  Crash1Runtime();

  const GuestProgramImage *guestProgramImage() const override;
  bool guestVramIsPicture(const Game &game) const override;

private:
  static const GuestProgramImage programImage_;
};

} // namespace crash1
