#pragma once

#include "game_runtime.h"

namespace crash1 {

// Process-lifetime owner of Crash 1's framework-facing behavior. The port currently reaches only a
// generated CPU boundary, so there are no game contexts, host overrides, or runtime products yet.
class Crash1Runtime final : public GameRuntime {
public:
  void *createContext(Core &core) override;
  void destroyContext(void *context) override;
  void registerOverrides(Game &game) override;
  void bootInit(Core &core) override;
};

} // namespace crash1
