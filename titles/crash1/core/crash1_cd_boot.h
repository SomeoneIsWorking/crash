#pragma once

#include "native_frame_loop_contract.h"

struct Core;

namespace crash1::cd_boot {

struct Program {
  crash::GuestFunctionRange initialize;
};

const Program &program();

// Install the native owner over retail libcd initialization. The guest body remains available
// for differential/A-B work; shipping boot cannot enter its VSync-driven controller wait.
void registerOverride(Core &core);

// Production behavior behind the override, public only so the focused test exercises the shipping
// state transition without needing an offline-generated guest corpus.
void initializeDriver(Core &core);

} // namespace crash1::cd_boot
