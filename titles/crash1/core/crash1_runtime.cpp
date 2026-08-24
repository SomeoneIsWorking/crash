#include "crash1_runtime.h"

namespace crash1 {

Crash1Runtime::Crash1Runtime()
    : BoundaryRuntime("crash1-runtime",
                      "native boot is unavailable: the independent oracle currently stops at Crash 1's "
                      "EnterCriticalSection syscall exception; a validated post-syscall resume model is "
                      "required before later boot") {}

bool Crash1Runtime::guestVramIsPicture(const Game &) const {
  return false;
}

} // namespace crash1
