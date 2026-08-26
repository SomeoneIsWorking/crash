#pragma once

#include "core.h"
#include "game.h"
#include "game_iface.h"

#include <cstdio>
#include <memory>
#include <type_traits>

template <typename Runtime> int verifyTitleRuntimeContract(const char *name, bool expectProgramImage) {
  static_assert(std::is_base_of_v<GameRuntime, Runtime>);
  static_assert(!std::is_base_of_v<LegacyGameRuntimeAdapter, Runtime>);

  Runtime runtime;
  psxport_install_game(runtime);

  auto game = std::make_unique<Game>();
  if (psxport_game_runtime() != &runtime || game->runtime != &runtime || game->core.runtime != &runtime) {
    std::fprintf(stderr, "%s did not own the installed framework seam\n", name);
    return 1;
  }
  if (game->core.cfg != nullptr || game->core.hooks != nullptr || runtime.legacyConfigForMigration() != nullptr ||
      runtime.legacyHooksForMigration() != nullptr) {
    std::fprintf(stderr, "%s exposed a legacy GameConfig/GameHooks view\n", name);
    return 1;
  }
  if (game->core.gameCtx != nullptr || game->frameDriver != nullptr || game->taskScheduler != nullptr) {
    std::fprintf(stderr, "%s invented an unmeasured context, frame driver, or scheduler\n", name);
    return 1;
  }
  if ((game->core.guestProgramImage != nullptr) != expectProgramImage) {
    std::fprintf(stderr, "%s installed the wrong GuestProgramImage ownership state\n", name);
    return 1;
  }
  if (runtime.guestVramIsPicture(*game)) {
    std::fprintf(stderr, "%s claimed guest VRAM was a picture without a measured frame producer\n", name);
    return 1;
  }
  const RenderCapabilities capabilities = runtime.renderCapabilities();
  if (capabilities.defaultPath != RenderPath::Native || !capabilities.nativeRenderPath ||
      !capabilities.temporalInterpolation || capabilities.playerPathCount() != 2) {
    std::fprintf(stderr, "%s did not declare the Crash lineage native/interpolation product target\n", name);
    return 1;
  }

  runtime.registerOverrides(*game);
  std::printf("%s: direct derived install; native/interpolation target; no legacy views, picture ownership, or "
              "unmeasured runtime products\n",
              name);
  return 0;
}
