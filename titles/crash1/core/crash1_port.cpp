#include "crash1_port.h"

#include "core.h"
#include "crash1_runtime.h"
#include "game.h"
#include "lightrec_executor.h"

#include <lucent/log.h>

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
    lucent::info("crash1", "Usage: {} [-h|--help]", argv[0]);
    lucent::info("crash1", "Run Crash Bandicoot through the host-owned native/Lightrec frame loop.");
    return EXIT_SUCCESS;
  }
  if (argc != 1) {
    lucent::error("crash1-boot", "usage: {}", argv[0]);
    return 2;
  }
  static Crash1Runtime runtime;
  psxport_install_game(runtime);

  auto game = std::make_unique<Game>();
  game->disc.env_key = "PSXPORT_CRASH1_DISC";
  Core *const core = &game->core;

  watchdog_init();
  load_exe(kDefaultExecutable, core);
  if (!core->lightrecExecutor().available()) {
    lucent::error("crash1-boot", "psxport was built without its Lightrec dynarec backend");
    return 2;
  }
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
