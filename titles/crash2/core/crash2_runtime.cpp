#include "crash2_runtime.h"

#include "crash2_frame_driver.h"
#include "platform_hle.h"

#include <memory>

namespace crash2 {

const GuestProgramImage Crash2Runtime::programImage_{
    .bss = {0x8005F2A4u, 0x8006F1F0u},
    .stackTopWordAddress = 0x8005F188u,
    .stackReserveWordAddress = 0x8005F184u,
    .heapBase = 0x8006F1F0u,
    .heapSizeStoreAddress = 0x8005CB2Cu,
    .heapBaseStoreAddress = 0x8005CB28u,
    .globalPointer = 0x8005F17Cu,
    .libcInitEntry = 0x8001144Cu,
    .gameMainEntry = 0x80049BD4u,
    .crt0Entry = 0x80049B2Cu,
    .residentText = {0x00010000u, 0x0005F800u},
    .backtraceText = {},
    .stackBias = {true, -8},
};

const PlatformHlePlan Crash2Runtime::platformPlan_ = crash::makeNativeFramePlatformPlan(Crash2FrameDriver::contract());

Crash2Runtime::Crash2Runtime()
    : BoundaryRuntime("crash2-runtime",
                      "native boot is unavailable: Crash 2 has no title-owned Lightrec boot services "
                      "or measured advancing frame driver; Crash 1 remains the active title") {}

const GuestProgramImage *Crash2Runtime::guestProgramImage() const {
  return &programImage_;
}

const PlatformHlePlan *Crash2Runtime::platformHlePlan() const {
  return &platformPlan_;
}

const GuestWidescreenProjection *Crash2Runtime::guestWidescreenProjection() const {
  // The policy answers which aspect the player selected; the owner behind it is what publishes a
  // guest projection (external/psxport/docs/presentation-contract.md, "Title-owned guest
  // widescreen"). Returning the policy without the three measured overrides installed would be the
  // half that advertises a capability the picture does not have - and the overrides cannot be
  // installed yet, because `BoundaryRuntime::registerOverrides` is final and Crash 2 still refuses to
  // boot. See docs/issues/0017 for the seam this leaves, which is a boot gap and not a projection gap.
  return &widescreen_;
}

bool Crash2Runtime::guestVramIsPicture(const Game &) const {
  return false;
}

std::unique_ptr<FrameDriver> Crash2Runtime::createFrameDriver(Game &) {
  return std::make_unique<Crash2FrameDriver>();
}

const crash::NativeFrameLoopContract &Crash2Runtime::nativeFrameLoopContract() const {
  return Crash2FrameDriver::contract();
}

} // namespace crash2
