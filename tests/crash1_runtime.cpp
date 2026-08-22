#include "crash1_runtime.h"

#include "core.h"
#include "game.h"
#include "game_iface.h"

#include <cstdio>
#include <memory>
#include <type_traits>

int main() {
  static_assert(std::is_base_of_v<GameRuntime, crash1::Crash1Runtime>);
  static_assert(!std::is_base_of_v<LegacyGameRuntimeAdapter, crash1::Crash1Runtime>);

  crash1::Crash1Runtime runtime;
  psxport_install_game(runtime);

  auto game = std::make_unique<Game>();
  if (psxport_game_runtime() != &runtime || game->runtime != &runtime || game->core.runtime != &runtime) {
    std::fputs("Crash1Runtime did not own the installed framework seam\n", stderr);
    return 1;
  }
  if (game->core.cfg != nullptr || game->core.hooks != nullptr || runtime.legacyConfigForMigration() != nullptr ||
      runtime.legacyHooksForMigration() != nullptr) {
    std::fputs("Crash1Runtime exposed a legacy GameConfig/GameHooks view\n", stderr);
    return 1;
  }
  if (game->core.gameCtx != nullptr || game->frameDriver != nullptr || game->taskScheduler != nullptr) {
    std::fputs("Crash1Runtime invented an unmeasured context, frame driver, or scheduler\n", stderr);
    return 1;
  }

  runtime.registerOverrides(*game);
  std::puts("Crash1Runtime: direct derived install; no legacy views or unmeasured runtime products");
  return 0;
}
