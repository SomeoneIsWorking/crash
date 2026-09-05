#include "crash1_callback_boot.h"

#include "core.h"
#include "dynarec_dispatch.h"

#include <array>
#include <cstdlib>
#include <lucent/log.h>

#ifndef CRASH1_CALLBACK_INITIALIZE_ENTRY
#error "CRASH1 callback initialization facts must come from titles/crash1/executable.json"
#endif
#ifndef CRASH1_CALLBACK_INITIALIZE_END
#error "CRASH1 callback initialization facts must come from titles/crash1/executable.json"
#endif

namespace crash1::callback_boot {
namespace {

constexpr Program kProgram{
    .initialize = {CRASH1_CALLBACK_INITIALIZE_ENTRY, CRASH1_CALLBACK_INITIALIZE_END},
};

constexpr std::uint32_t kEnterCritical = 0x8003E1F8u;
constexpr std::uint32_t kOpenEvent = 0x8003E1A8u;
constexpr std::uint32_t kExitCritical = 0x8003E208u;
constexpr std::uint32_t kCloseEvent = 0x8003E1D8u;
constexpr std::uint32_t kInitializePad = 0x80051444u;
constexpr std::uint32_t kStartPad = 0x80051454u;
constexpr std::uint32_t kChangeClearPad = 0x8003E198u;

struct EventSpec {
  std::uint32_t descriptor;
  std::uint32_t mode;
  std::uint32_t handleOffset;
  std::uint32_t continuation;
  std::uint32_t closeContinuation;
};

constexpr std::array<EventSpec, 8> kEvents{{
    {0xF4000001u, 0x0004u, 712u, 0x8003CBC4u, 0x8003CCD8u},
    {0xF4000001u, 0x8000u, 716u, 0x8003CBE0u, 0x8003CCE4u},
    {0xF4000001u, 0x0100u, 720u, 0x8003CBFCu, 0x8003CCF0u},
    {0xF4000001u, 0x2000u, 724u, 0x8003CC18u, 0x8003CCFCu},
    {0xF0000011u, 0x0004u, 752u, 0x8003CC34u, 0x8003CD08u},
    {0xF0000011u, 0x8000u, 756u, 0x8003CC50u, 0x8003CD14u},
    {0xF0000011u, 0x0100u, 764u, 0x8003CC6Cu, 0x8003CD20u},
    {0xF0000011u, 0x2000u, 768u, 0x8003CC88u, 0x8003CD2Cu},
}};

static_assert(kProgram.initialize.valid());

void initializeOverride(Core *core) {
  initializeDriver(*core, [](Core *target, std::uint32_t address) {
    crash::dynarec::requireGuestReturn(crash::dynarec::callGuest(*target, address),
                                       "Crash 1 callback initialization leaf");
  });
}

void dispatchAt(Core &core, MainDispatch dispatch, std::uint32_t address, std::uint32_t continuation) {
  core.r[31] = continuation;
  dispatch(&core, address);
}

} // namespace

const Program &program() {
  return kProgram;
}

void registerOverride(Core &core) {
  if (!crash::dynarec::installOverride(
          core, kProgram.initialize.begin, "Crash 1 callback initialization", initializeOverride)) {
    std::abort();
  }
}

void initializeDriver(Core &core, MainDispatch dispatch) {
  if (dispatch == nullptr) {
    lucent::error("crash1-callback", "Crash 1 callback boot owner received a null main dispatcher");
    std::abort();
  }

  // The verified SCUS_949.00 body [0x8003CB9C,0x8003CD3C) creates eight BIOS events, initializes
  // the native pad service, then closes those temporary handles. Its four VSync(5) calls are only
  // hardware-settle delays between synchronous initialization steps. The host owns both that
  // hardware and all display timing, so preserve every state-producing call and make no guest
  // display-wait call at all.
  const std::uint32_t incomingStack = core.r[29];
  const std::uint32_t incomingReturn = core.r[31];
  core.r[29] -= 24u;
  core.mem_w32(core.r[29] + 16u, incomingReturn);

  dispatchAt(core, dispatch, kEnterCritical, 0x8003CBACu);
  for (const EventSpec &event : kEvents) {
    core.r[4] = event.descriptor;
    core.r[5] = event.mode;
    core.r[6] = 0x2000u;
    core.r[7] = 0u;
    dispatchAt(core, dispatch, kOpenEvent, event.continuation);
    core.mem_w32(core.r[28] + event.handleOffset, core.r[2]);
  }
  dispatchAt(core, dispatch, kExitCritical, 0x8003CC94u);

  core.r[4] = 1u;
  dispatchAt(core, dispatch, kInitializePad, 0x8003CCA4u);
  dispatchAt(core, dispatch, kStartPad, 0x8003CCB4u);
  dispatchAt(core, dispatch, kChangeClearPad, 0x8003CCC4u);

  for (const EventSpec &event : kEvents) {
    core.r[4] = core.mem_r32(core.r[28] + event.handleOffset);
    dispatchAt(core, dispatch, kCloseEvent, event.closeContinuation);
  }

  core.r[31] = core.mem_r32(core.r[29] + 16u);
  core.r[29] = incomingStack;
}

} // namespace crash1::callback_boot
