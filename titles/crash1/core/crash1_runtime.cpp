#include "crash1_runtime.h"

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

Crash1Runtime::Crash1Runtime()
    : BoundaryRuntime("crash1-runtime",
                      "native boot is unavailable: Crash 1 is verified through its first B(56h) pre-HLE "
                      "dispatch; its C0-table effect and the following A(44h) state remain unverified") {}

const GuestProgramImage *Crash1Runtime::guestProgramImage() const {
  return &programImage_;
}

bool Crash1Runtime::guestVramIsPicture(const Game &) const {
  return false;
}

} // namespace crash1
