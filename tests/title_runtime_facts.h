#pragma once

#include "platform_hle.h"

#include <cstdio>

template <typename Runtime> int writeTitleRuntimeFacts(const char *runtimeName) {
  const Runtime runtime;
  const GuestProgramImage *image = runtime.guestProgramImage();
  if (image == nullptr) {
    std::fprintf(stderr, "REFUSED: %s supplies no GuestProgramImage\n", runtimeName);
    return 2;
  }
  const auto &frame = runtime.nativeFrameLoopContract();
  const PlatformHlePlan *platform = runtime.platformHlePlan();
  if (platform == nullptr) {
    std::fprintf(stderr, "REFUSED: %s supplies no PlatformHlePlan\n", runtimeName);
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
  std::printf("vsync_begin=0x%08X\n", frame.guestVSync.begin);
  std::printf("vsync_end=0x%08X\n", frame.guestVSync.end);
  std::printf("platform_vsync=0x%08X\n", platform->vsyncAddress);
  std::printf("platform_window_begin=0x%08X\n", platform->windowLo[0]);
  std::printf("platform_window_end=0x%08X\n", platform->windowHi[0]);
  std::puts("runtime facts: 20 field(s)");
  return 0;
}
