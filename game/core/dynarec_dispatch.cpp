#include "dynarec_dispatch.h"

#include "core.h"

#include <cstdlib>
#include <lucent/log.h>

namespace crash::dynarec {

bool installOverride(Core &core, std::uint32_t address, std::string_view name, psx::cpu::NativeFunction function) {
  const auto image = core.currentImageIdentity(address);
  if (!image) {
    lucent::error("crash-dynarec", "override '{}' has no unambiguous active image at 0x{:08X}", name, address);
    return false;
  }
  return core.nativeDispatcher().install({{*image, address}, name, function});
}

psx::cpu::ExecutionResult callGuest(Core &core, std::uint32_t address) {
  return psx::cpu::dispatchGuest(core, address, psx::cpu::ExecutionBudget::currentTurn(core));
}

psx::cpu::ExecutionResult executeTurn(Core &core, std::uint32_t address) {
  return psx::cpu::dispatchGuestUntilExit(core, address, psx::cpu::ExecutionBudget::currentTurn(core));
}

psx::cpu::ExecutionResult callOriginal(Core &core, std::uint32_t address) {
  return psx::cpu::callOriginal(core, address, psx::cpu::ExecutionBudget::currentTurn(core));
}

void requireGuestReturn(const psx::cpu::ExecutionResult &result, std::string_view owner) {
  if (!psx::cpu::requireGuestReturn(result, owner)) {
    std::abort();
  }
}

} // namespace crash::dynarec
