#pragma once

#include "native_frame_loop_contract.h"

#include <cstdint>

struct Core;

namespace crash1::callback_boot {

struct Program {
  crash::GuestFunctionRange initialize;
};

using MainDispatch = void (*)(Core *, std::uint32_t);

const Program &program();

// Install the native owner over the retail callback/event initializer. The generated body remains
// available for differential/A-B work; shipping boot cannot enter its four VSync-driven delays.
void registerOverride();

// Production behavior behind the override, with dispatch injectable so the focused test exercises
// the exact shipping sequence rather than a second implementation.
void initializeDriver(Core &core, MainDispatch dispatch);

} // namespace crash1::callback_boot
