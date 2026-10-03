// Crash 1's product entry point: argument handling, then composition. Everything between here and
// the guest's first instruction belongs to `crash1::boot::ProductBoot`.
#include "crash1_boot.h"
#include "crash1_runtime.h"

#include <lucent/log.h>

#include <cstdlib>
#include <cstring>
#include <string_view>

namespace {

bool isHelpRequest(int argc, char **argv) {
  return argc == 2 && (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0);
}

} // namespace

int main(int argc, char **argv) {
  if (isHelpRequest(argc, argv)) {
    lucent::info("crash1", "Usage: {} [-h|--help]", argv[0]);
    lucent::info("crash1", "Run Crash Bandicoot through the host-owned native/Lightrec frame loop.");
    return EXIT_SUCCESS;
  }
  if (argc != 1) {
    lucent::error("crash1-boot", "usage: {}", argv[0]);
    return 2;
  }

  // The runtime lives for the whole process: the framework captures it, and every native owner
  // reaches this title's facts and state through it.
  crash1::Crash1Runtime runtime;
  crash1::boot::ProductBoot boot{runtime};
  return boot.run(crash1::boot::defaultExecutablePath());
}