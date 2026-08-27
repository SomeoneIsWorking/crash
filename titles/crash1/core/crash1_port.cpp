#include "crash1_port.h"

#include "core.h"
#include "crash1_runtime.h"
#include "game.h"
#include "resident_program.h"

#include <lucent/log.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

extern "C" {
void mdec_init();
void spu_init();
void watchdog_init();
}

void gte_init();
void load_exe(const char *path, Core *core);
void native_boot_run(Core *core);

namespace crash1 {
namespace {

constexpr const char *kDefaultExecutable = "scratch/bin/crash1/SCUS_949.00";

} // namespace

int runPort(int argc, char **argv) {
  if (argc == 2 && (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0)) {
    std::printf("Usage: %s [-h|--help]\n", argv[0]);
    std::puts("Run Crash Bandicoot through the host-owned native frame loop.");
    return EXIT_SUCCESS;
  }
  if (argc != 1) {
    lucent::error("crash1-boot", "usage: {}", argv[0]);
    return 2;
  }
  static Crash1Runtime runtime;
  crash::installResidentRuntime(runtime);

  auto game = std::make_unique<Game>();
  game->disc.env_key = "PSXPORT_CRASH1_DISC";
  Core *const core = &game->core;

  watchdog_init();
  load_exe(kDefaultExecutable, core);
  gte_init();
  mdec_init();
  spu_init();
  game->spu_audio.init();
  game->gpu.gpu_native_init();
  game->pad.overridesInit();

  runtime.registerOverrides(*game);
  lucent::info("crash1-boot", "entering the host-owned Crash 1 boot and frame loop");
  native_boot_run(core);
  lucent::info("crash1-boot", "Crash 1 native loop returned");
  return EXIT_SUCCESS;
}

} // namespace crash1
