#include "crash1_port.h"

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

#ifndef CRASH1_FIRST_SYSCALL_ENTRY
#error "CRASH1_FIRST_SYSCALL_ENTRY must be generated from titles/crash1/executable.json"
#endif

void stopAtFirstMeasuredSyscall(Core *core) {
  const crash::EnterCriticalResult result = crash::runEnterCriticalFrontier(*core, kCodeword, gen_func_8003E1F8);
  if (!result.accepted) {
    lucent::error("crash1-boot", "{}", result.detail);
    std::exit(2);
  }
  lucent::info("crash1-boot",
               "{} at 0x{:08X}; IRQ {} -> {}. Later boot remains blocked on independent "
               "Cause/EPC validation and syscall resume.",
               result.detail,
               result.observation.boundary,
               result.observation.irqBefore,
               result.observation.irqAfter);
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
  return crash::runResidentProgram({
      .runtime = runtime,
      .codeword = kCodeword,
      .executable = kDefaultExecutable,
      .entry = image->crt0Entry,
      .boundary = CRASH1_FIRST_SYSCALL_ENTRY,
      .boundaryHandler = stopAtFirstMeasuredSyscall,
  });
}

} // namespace crash1
