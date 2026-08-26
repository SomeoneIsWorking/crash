#pragma once

#include "game_runtime.h"

#include <string_view>

namespace crash {

// Framework-integration owner for titles whose measured execution frontier is not yet a native
// boot. This class owns no Crash engine semantics: title runtimes inherit it only to share the
// refusal invariant and the absence of unmeasured runtime products.
class BoundaryRuntime : public GameRuntime {
public:
  RenderCapabilities renderCapabilities() const final;
  void *createContext(Core &core) final;
  void destroyContext(void *context) final;
  void registerOverrides(Game &game) final;
  void bootInit(Core &core) final;

protected:
  BoundaryRuntime(std::string_view logDomain, std::string_view blockedReason);

private:
  std::string_view logDomain_;
  std::string_view blockedReason_;
};

} // namespace crash
