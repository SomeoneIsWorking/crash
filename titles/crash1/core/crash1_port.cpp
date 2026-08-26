#include "crash1_port.h"

#include "crash1_boot_frontier.h"
#include "crash1_runtime.h"
#include "enter_critical_frontier.h"
#include "rec_decls.h"
#include "resident_program.h"

#include <lucent/log.h>

#include <cstdlib>
#include <string_view>

namespace crash1 {
namespace {

constexpr std::string_view kCodeword = "SCUS-94900";
constexpr const char *kDefaultExecutable = "scratch/bin/crash1/SCUS_949.00";

void resumeFromFirstMeasuredSyscall(Core *core) {
  const crash::EnterCriticalResult result = crash::runEnterCriticalFrontier(*core, kCodeword, gen_func_8003E1F8);
  if (!result.accepted) {
    lucent::error("crash1-boot", "{}", result.detail);
    std::exit(2);
  }
  lucent::info("crash1-boot",
               "{} at 0x{:08X}; IRQ {} -> {}. Resuming toward the measured B(56h) boundary.",
               result.detail,
               result.observation.boundary,
               result.observation.irqBefore,
               result.observation.irqAfter);
}

void stopAfterFirstMeasuredBiosDispatch(Core *core) {
  const BiosDispatchResult result = checkPostGetC0Dispatch(core->pc, core->r[9], core->r[31]);
  if (!result.accepted) {
    lucent::error("crash1-boot", "{}", result.detail);
    std::exit(2);
  }
  lucent::info("crash1-boot",
               "{}. B(56h) executed, but serialized A(44h) register/RAM equality and later boot remain unverified.",
               result.detail);
  std::exit(EXIT_SUCCESS);
}

} // namespace

int runPort(int argc, char **argv) {
  if (argc != 1) {
    lucent::error("crash1-boot", "usage: {}", argv[0]);
    return 2;
  }
  static Crash1Runtime runtime;
  const GuestProgramImage *image = runtime.guestProgramImage();
  if (image == nullptr) {
    lucent::error("crash1-boot", "Crash1Runtime supplies no measured guest program image");
    return 2;
  }
  return crash::runResidentProgram(makeBootFrontierProgram(
      runtime, kDefaultExecutable, resumeFromFirstMeasuredSyscall, stopAfterFirstMeasuredBiosDispatch));
}

} // namespace crash1
