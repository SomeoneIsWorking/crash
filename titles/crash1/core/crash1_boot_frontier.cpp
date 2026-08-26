#include "crash1_boot_frontier.h"

#include "crash1_runtime.h"

#include <cstdio>

namespace crash1 {

#ifndef CRASH1_FIRST_SYSCALL_ENTRY
#error "CRASH1_FIRST_SYSCALL_ENTRY must be generated from titles/crash1/executable.json"
#endif
#ifndef CRASH1_FIRST_BIOS_DISPATCH_ADDRESS
#error "CRASH1_FIRST_BIOS_DISPATCH_ADDRESS must be generated from titles/crash1/executable.json"
#endif
#ifndef CRASH1_FIRST_BIOS_DISPATCH_FUNCTION
#error "CRASH1_FIRST_BIOS_DISPATCH_FUNCTION must be generated from titles/crash1/executable.json"
#endif
#ifndef CRASH1_FIRST_BIOS_DISPATCH_RETURN_ADDRESS
#error "CRASH1_FIRST_BIOS_DISPATCH_RETURN_ADDRESS must be generated from titles/crash1/executable.json"
#endif
#ifndef CRASH1_POST_GET_C0_DISPATCH_ADDRESS
#error "CRASH1_POST_GET_C0_DISPATCH_ADDRESS must be generated from titles/crash1/executable.json"
#endif
#ifndef CRASH1_POST_GET_C0_DISPATCH_FUNCTION
#error "CRASH1_POST_GET_C0_DISPATCH_FUNCTION must be generated from titles/crash1/executable.json"
#endif
#ifndef CRASH1_POST_GET_C0_DISPATCH_RETURN_ADDRESS
#error "CRASH1_POST_GET_C0_DISPATCH_RETURN_ADDRESS must be generated from titles/crash1/executable.json"
#endif

namespace {

constexpr const char *kCodeword = "SCUS-94900";
constexpr BootFrontierFacts kFacts{
    .firstSyscall = CRASH1_FIRST_SYSCALL_ENTRY,
    .firstBiosDispatch = CRASH1_FIRST_BIOS_DISPATCH_ADDRESS,
    .firstBiosFunction = CRASH1_FIRST_BIOS_DISPATCH_FUNCTION,
    .firstBiosReturnAddress = CRASH1_FIRST_BIOS_DISPATCH_RETURN_ADDRESS,
    .postGetC0Dispatch = CRASH1_POST_GET_C0_DISPATCH_ADDRESS,
    .postGetC0Function = CRASH1_POST_GET_C0_DISPATCH_FUNCTION,
    .postGetC0ReturnAddress = CRASH1_POST_GET_C0_DISPATCH_RETURN_ADDRESS,
};

} // namespace

BootFrontierFacts bootFrontierFacts() {
  return kFacts;
}

crash::ResidentProgram makeBootFrontierProgram(Crash1Runtime &runtime,
                                               const char *executable,
                                               RecOverrideFn syscallTransition,
                                               RecOverrideFn biosBoundary) {
  const GuestProgramImage *image = runtime.guestProgramImage();
  return {
      .runtime = runtime,
      .codeword = kCodeword,
      .executable = executable,
      .entry = image == nullptr ? 0u : image->crt0Entry,
      .boundary = kFacts.postGetC0Dispatch,
      .boundaryHandler = biosBoundary,
      .boundaryKind = crash::ResidentBoundaryKind::DynamicDispatch,
      .transitionBoundary = kFacts.firstSyscall,
      .transitionHandler = syscallTransition,
  };
}

BiosDispatchResult checkFirstBiosDispatch(std::uint32_t address, std::uint32_t function, std::uint32_t returnAddress) {
  if (address == kFacts.firstBiosDispatch && function == kFacts.firstBiosFunction &&
      returnAddress == kFacts.firstBiosReturnAddress) {
    return {.accepted = true, .detail = "reached the measured B(56h) pre-HLE dispatch boundary"};
  }

  char detail[256]{};
  std::snprintf(detail,
                sizeof(detail),
                "first BIOS dispatch disagrees: pc=0x%08X function=0x%08X ra=0x%08X; "
                "expected pc=0x%08X function=0x%08X ra=0x%08X",
                address,
                function,
                returnAddress,
                kFacts.firstBiosDispatch,
                kFacts.firstBiosFunction,
                kFacts.firstBiosReturnAddress);
  return {.detail = detail};
}

BiosDispatchResult checkPostGetC0Dispatch(std::uint32_t address, std::uint32_t function, std::uint32_t returnAddress) {
  if (address == kFacts.postGetC0Dispatch && function == kFacts.postGetC0Function &&
      returnAddress == kFacts.postGetC0ReturnAddress) {
    return {.accepted = true, .detail = "reached the measured post-B(56h) A(44h) pre-HLE dispatch boundary"};
  }

  char detail[256]{};
  std::snprintf(detail,
                sizeof(detail),
                "post-B(56h) BIOS dispatch disagrees: pc=0x%08X function=0x%08X ra=0x%08X; "
                "expected pc=0x%08X function=0x%08X ra=0x%08X",
                address,
                function,
                returnAddress,
                kFacts.postGetC0Dispatch,
                kFacts.postGetC0Function,
                kFacts.postGetC0ReturnAddress);
  return {.detail = detail};
}

} // namespace crash1
