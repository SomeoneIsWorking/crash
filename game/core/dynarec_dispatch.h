#pragma once

#include "execution_exit.h"
#include "native_dispatch.h"

#include <cstdint>
#include <string_view>

class Core;

namespace crash::dynarec {

bool installOverride(Core &core, std::uint32_t address, std::string_view name, psx::cpu::NativeFunction function);
// Execute an open-ended guest turn until a typed host/runtime boundary or the current budget.
psx::cpu::ExecutionResult executeTurn(Core &core, std::uint32_t address);
// Call one guest function and require callers to inspect its exact return/exit result.
psx::cpu::ExecutionResult callGuest(Core &core, std::uint32_t address);
psx::cpu::ExecutionResult callOriginal(Core &core, std::uint32_t address);
void requireGuestReturn(const psx::cpu::ExecutionResult &result, std::string_view owner);

} // namespace crash::dynarec
