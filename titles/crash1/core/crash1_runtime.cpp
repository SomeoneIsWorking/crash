#include "crash1_runtime.h"

#include <lucent/log.h>

#include <cstdlib>

namespace crash1 {

void *Crash1Runtime::createContext(Core &) {
  return nullptr;
}

void Crash1Runtime::destroyContext(void *) {}

void Crash1Runtime::registerOverrides(Game &) {}

void Crash1Runtime::bootInit(Core &) {
  lucent::error("crash1-runtime",
                "native boot is unavailable: the independent oracle currently stops at Crash 1's "
                "EnterCriticalSection syscall exception; a validated post-syscall resume model is "
                "required before later boot");
  std::abort();
}

} // namespace crash1
