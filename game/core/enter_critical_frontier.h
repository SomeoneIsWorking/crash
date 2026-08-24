#pragma once

#include "recomp_iface.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace crash {

struct EnterCriticalObservation {
  std::uint32_t boundary{};
  std::uint32_t selector{};
  std::uint32_t returnValue{};
  int irqBefore{};
  int irqAfter{};
  std::uint32_t statusBefore{};
  std::uint32_t statusAfter{};
};

struct EnterCriticalResult {
  bool accepted{};
  EnterCriticalObservation observation{};
  std::string detail;
};

// Execute the measured addiu-a0-1/syscall-0 wrapper through the generated body and require the
// shipping HLE's observed EnterCriticalSection transition. Product and oracle harness share this
// exact discriminator and transition check.
EnterCriticalResult runEnterCriticalFrontier(Core &core, std::string_view codeword, RecOverrideFn generatedBody);

} // namespace crash
