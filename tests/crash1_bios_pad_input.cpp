#include "crash1_bios_pad_input.h"

#include "core.h"

#include <cstdint>
#include <cstdio>
#include <memory>

namespace {

bool expectWord(Core &core, std::uint16_t activeLowButtons, std::uint32_t expected, const char *label) {
  crash1::bios_pad_input::publishPrimary(core, activeLowButtons);
  const std::uint32_t actual = core.mem_r32(crash1::bios_pad_input::wordAddress());
  if (actual == expected) {
    return true;
  }
  std::fprintf(stderr, "%s: expected 0x%08X, got 0x%08X\n", label, expected, actual);
  return false;
}

} // namespace

int main() {
  auto core = std::make_unique<Core>();
  bool ok = true;

  ok &= expectWord(*core, 0xFFFFu, 0xFFFFFFFFu, "released input");
  ok &= expectWord(*core, 0xBFFFu, 0xFFBFFFFFu, "Cross input");
  ok &= expectWord(*core, 0xFFF7u, 0xF7FFFFFFu, "Start input");

  const std::uint32_t word = core->mem_r32(crash1::bios_pad_input::wordAddress());
  const std::uint16_t crashLogicalButtons = static_cast<std::uint16_t>(~(word >> 16u));
  if (crashLogicalButtons != 0x0800u) {
    std::fprintf(stderr, "Crash PadUpdate interpretation: expected Start 0x0800, got 0x%04X\n", crashLogicalButtons);
    ok = false;
  }

  return ok ? 0 : 1;
}
