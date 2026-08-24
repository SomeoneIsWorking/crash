#include "crash2_runtime.h"

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

Crash2Runtime::Crash2Runtime()
    : BoundaryRuntime("crash2-runtime",
                      "native boot is unavailable: Crash 2 execution has been independently verified "
                      "only through its resident EnterCriticalSection boundary; syscall continuation "
                      "is required before later boot") {}

const GuestProgramImage *Crash2Runtime::guestProgramImage() const {
  return &programImage_;
}

bool Crash2Runtime::guestVramIsPicture(const Game &) const {
  return false;
}

} // namespace crash2
