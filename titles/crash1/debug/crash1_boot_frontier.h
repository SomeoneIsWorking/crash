#pragma once

#include <cstdint>
#include <string>

namespace crash1 {

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

BiosDispatchResult checkFirstBiosDispatch(std::uint32_t address, std::uint32_t function, std::uint32_t returnAddress);
BiosDispatchResult checkPostGetC0Dispatch(std::uint32_t address, std::uint32_t function, std::uint32_t returnAddress);

} // namespace crash1
