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
                "native boot is unavailable: Crash 1 execution currently ends at the measured BIOS "
                "exception boundary 0xBFC00180");
  std::abort();
}

} // namespace crash1
