#include "crash1_boot.h"

#include "core.h"
#include "crash1_executable.h"
#include "crash1_runtime.h"
#include "game.h"
#include "lightrec_executor.h"
#include "machine.h"

#include <lucent/log.h>

#include <cstdlib>

#include "c_subsys.h"
#include "native_boot.h"

namespace crash1::boot {
namespace {

constexpr const char *kLogDomain = "crash1-boot";
const std::filesystem::path kDefaultExecutablePath{"scratch/bin/crash1/SCUS_949.00"};

} // namespace

const std::filesystem::path &defaultExecutablePath() {
  return kDefaultExecutablePath;
}

ProductBoot::ProductBoot(Crash1Runtime &runtime) : runtime_(runtime) {
  // The runtime is installed BEFORE the Game is constructed: `Game` captures it, and every owner
  // below reaches this title through it.
  psxport_install_game(runtime_);
  game_ = std::make_unique<Game>();
  game_->disc.env_key = kDiscEnvironmentKey;
}

// `Game` is complete here, so the unique_ptr's deleter is instantiated once, in this translation
// unit, rather than at every include of the header.
ProductBoot::~ProductBoot() = default;

void ProductBoot::bindFrameworkDevices() {
  // The framework's measured bind order and its per-instance device binds, in one call, before any
  // guest code can run. Construction binds nothing; this step is adopted deliberately.
  psx::Machine(*game_).bindDevices();
}

void ProductBoot::logExecutionCounters() const {
  const auto &execution = game_->core.lightrecExecutor().counters();
  lucent::info(kLogDomain,
               "Lightrec run-end: translated_blocks={} executed_blocks={} executed_instructions={} "
               "fallback_blocks={} fallback_instructions={}",
               execution.translatedBlocks,
               execution.executedBlocks,
               execution.executedInstructions,
               execution.fallback.calls,
               execution.fallback.instructions);
}

int ProductBoot::run(const std::filesystem::path &executable) {
  // The watchdog is armed before anything is mapped, so a hang in image admission is still a
  // watchdog event rather than a silent stop.
  watchdog_init();

  const auto loaded = loadResidentExecutable(game_->core, executable);
  if (!loaded) {
    lucent::error(
        kLogDomain, "cannot load authenticated Crash 1 executable '{}': {}", executable.string(), loaded.detail);
    return EXIT_FAILURE;
  }
  lucent::info(kLogDomain,
               "authenticated Crash 1 executable: entry 0x{:08X}, text 0x{:08X}+0x{:X}",
               loaded.image.entry,
               loaded.image.textAddress,
               loaded.image.textBytes);

  if (!game_->core.lightrecExecutor().available()) {
    lucent::error(kLogDomain, "psxport was built without its Lightrec dynarec backend");
    return 2;
  }

  bindFrameworkDevices();
  runtime_.registerOverrides(*game_);
  lucent::info(kLogDomain, "entering the host-owned Crash 1 boot and frame loop");
  native_boot_run(&game_->core);
  logExecutionCounters();
  lucent::info(kLogDomain, "Crash 1 native loop returned");
  return EXIT_SUCCESS;
}

} // namespace crash1::boot