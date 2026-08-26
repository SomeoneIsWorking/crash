#pragma once

#include "resident_program.h"

#include <cstdint>
#include <string>

namespace crash1 {

class Crash1Runtime;

struct BootFrontierFacts {
  std::uint32_t firstSyscall;
  std::uint32_t firstBiosDispatch;
  std::uint32_t firstBiosFunction;
  std::uint32_t firstBiosReturnAddress;
  std::uint32_t postGetC0Dispatch;
  std::uint32_t postGetC0Function;
  std::uint32_t postGetC0ReturnAddress;
};

struct BiosDispatchResult {
  bool accepted{};
  std::string detail;
};

BootFrontierFacts bootFrontierFacts();

// Compose the product's measured resident execution spine: the generated syscall wrapper is a
// returning transition, B(56h) executes through the shipping HLE, and the following A(44h) dispatch
// is the final pre-HLE observation boundary.
crash::ResidentProgram makeBootFrontierProgram(Crash1Runtime &runtime,
                                               const char *executable,
                                               RecOverrideFn syscallTransition,
                                               RecOverrideFn biosBoundary);

BiosDispatchResult checkFirstBiosDispatch(std::uint32_t address, std::uint32_t function, std::uint32_t returnAddress);
BiosDispatchResult checkPostGetC0Dispatch(std::uint32_t address, std::uint32_t function, std::uint32_t returnAddress);

} // namespace crash1
