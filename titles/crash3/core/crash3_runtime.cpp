#include "crash3_runtime.h"

namespace crash3 {

const GuestProgramImage Crash3Runtime::programImage_{
    .bss = {0x80060A38u, 0x8006EA78u},
    .stackTopWordAddress = 0x80060884u,
    .stackReserveWordAddress = 0x80060880u,
    .heapBase = 0x8006EA78u,
    .heapSizeStoreAddress = 0x8005E1ACu,
    .heapBaseStoreAddress = 0x8005E1A8u,
    .globalPointer = 0x80060878u,
    .libcInitEntry = 0x800112B8u,
    .gameMainEntry = 0x80048AA0u,
    .crt0Entry = 0x800489F8u,
    .residentText = {0x00010000u, 0x00061000u},
    .backtraceText = {},
    .stackBias = {true, -8},
};

Crash3Runtime::Crash3Runtime()
    : BoundaryRuntime("crash3-runtime",
                      "native boot is unavailable: SCUS-94244 generated execution reaches its "
                      "EnterCriticalSection syscall boundary, but independent post-syscall equality "
                      "is not yet available") {}

const GuestProgramImage *Crash3Runtime::guestProgramImage() const {
  return &programImage_;
}

} // namespace crash3
