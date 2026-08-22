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
    .gameMainEntry = 0,
    .crt0Entry = 0x800489F8u,
    .residentText = {0x00010000u, 0x00061000u},
    .backtraceText = {},
    .stackBias = {true, -8},
};

Crash3Runtime::Crash3Runtime()
    : BoundaryRuntime("crash3-runtime",
                      "native boot is unavailable: Crash 3 execution has been independently verified "
                      "only through its first crt0 call boundary; a generated-path comparison is "
                      "required before later boot") {}

const GuestProgramImage *Crash3Runtime::guestProgramImage() const {
  return &programImage_;
}

} // namespace crash3
