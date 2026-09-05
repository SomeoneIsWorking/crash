#include "crash1_runtime.h"

#include "core.h"
#include "crash1_callback_boot.h"
#include "crash1_cd_boot.h"
#include "crash1_disc_index_io.h"
#include "crash1_frame_driver.h"
#include "crash1_gpu_watchdog.h"
#include "dynarec_dispatch.h"
#include "game.h"
#include "platform_hle.h"

#include <cstdlib>
#include <lucent/log.h>
#include <memory>

#ifndef CRASH1_STATIC_CONSTRUCTORS_ENTRY
#error "CRASH1 native boot facts must come from titles/crash1/executable.json"
#endif

namespace crash1 {

const GuestProgramImage Crash1Runtime::programImage_{
    .bss = {0x80056598u, 0x80061A78u},
    .stackTopWordAddress = 0x80056408u,
    .stackReserveWordAddress = 0x80056404u,
    .heapBase = 0x80061A78u,
    .heapSizeStoreAddress = 0x800538F0u,
    .heapBaseStoreAddress = 0x800538ECu,
    .globalPointer = 0x800563FCu,
    .libcInitEntry = 0x80011A18u,
    .gameMainEntry = 0x8003E0C0u,
    .crt0Entry = 0x8003E018u,
    .residentText = {0x00010000u, 0x00056800u},
    .backtraceText = {},
    .stackBias = {true, -8},
};

const PlatformHlePlan Crash1Runtime::platformPlan_ = crash::makeNativeFramePlatformPlan(Crash1FrameDriver::contract());

Crash1Runtime::Crash1Runtime() = default;

RenderCapabilities Crash1Runtime::renderCapabilities() const {
  return RenderCapabilities::interpolatedNative();
}

void *Crash1Runtime::createContext(Core &) {
  return nullptr;
}

void Crash1Runtime::destroyContext(void *) {}

void Crash1Runtime::registerOverrides(Game &game) {
  cd_boot::registerOverride(game.core);
  disc_index_io::registerOverrides(game.core);
  callback_boot::registerOverride(game.core);
  gpu_watchdog::registerOverrides(game.core);
  Crash1FrameDriver::installOverrides(game);
}

void Crash1Runtime::bootInit(Core &core) {
  const std::uint32_t entries[]{CRASH1_STATIC_CONSTRUCTORS_ENTRY, CRASH1_INIT_ENTRY};

  // Retail C main 0x80011D88 performs these three operations before entering CoreLoop 0x80011FC4.
  // Shared dc_boot_init already applied the measured crt0/libc group, so dispatching C main itself
  // would re-enter the guest-owned infinite loop and then run shutdown behind the host's back.
  crash::dynarec::requireGuestReturn(crash::dynarec::callGuest(core, entries[0]), "Crash 1 static constructors");
  core.mem_w32(CRASH1_USE_CD_ADDRESS, 1u);
  crash::dynarec::requireGuestReturn(crash::dynarec::callGuest(core, entries[1]), "Crash 1 Init");
}

const GuestProgramImage *Crash1Runtime::guestProgramImage() const {
  return &programImage_;
}

const PlatformHlePlan *Crash1Runtime::platformHlePlan() const {
  return &platformPlan_;
}

bool Crash1Runtime::guestVramIsPicture(const Game &) const {
  return false;
}

std::unique_ptr<FrameDriver> Crash1Runtime::createFrameDriver(Game &game) {
  return std::make_unique<Crash1FrameDriver>(game);
}

const crash::NativeFrameLoopContract &Crash1Runtime::nativeFrameLoopContract() const {
  return Crash1FrameDriver::contract();
}

} // namespace crash1
