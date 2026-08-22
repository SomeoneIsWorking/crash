#include "crash2_runtime.h"

#include <cstdio>

int main() {
  const crash2::Crash2Runtime runtime;
  const GuestProgramImage *image = runtime.guestProgramImage();
  if (image == nullptr) {
    std::fputs("REFUSED: Crash2Runtime supplies no GuestProgramImage\n", stderr);
    return 2;
  }

  std::printf("bss_begin=0x%08X\n", image->bss.begin);
  std::printf("bss_end=0x%08X\n", image->bss.end);
  std::printf("stack_top_word=0x%08X\n", image->stackTopWordAddress);
  std::printf("stack_reserve_word=0x%08X\n", image->stackReserveWordAddress);
  std::printf("heap_base=0x%08X\n", image->heapBase);
  std::printf("heap_size_store=0x%08X\n", image->heapSizeStoreAddress);
  std::printf("heap_base_store=0x%08X\n", image->heapBaseStoreAddress);
  std::printf("global_pointer=0x%08X\n", image->globalPointer);
  std::printf("libc_init=0x%08X\n", image->libcInitEntry);
  std::printf("game_main=0x%08X\n", image->gameMainEntry);
  std::printf("crt0_entry=0x%08X\n", image->crt0Entry);
  std::printf("resident_begin=0x%08X\n", image->residentText.begin);
  std::printf("resident_end=0x%08X\n", image->residentText.end);
  std::printf("stack_bias_declared=%u\n", image->stackBias.declared ? 1u : 0u);
  std::printf("stack_bias=%d\n", image->stackBias.bytes);
  std::puts("runtime facts: 15 field(s)");
  return 0;
}
