#include "crash1_bios_pad_input.h"

#include "core.h"

#ifndef CRASH1_BIOS_PAD_READ_WORD_ADDRESS
#error "Crash 1 BIOS pad facts must come from titles/crash1/executable.json"
#endif

namespace crash1::bios_pad_input {
namespace {

constexpr std::uint16_t toBiosHalfword(std::uint16_t activeLowButtons) {
  return static_cast<std::uint16_t>((activeLowButtons << 8u) | (activeLowButtons >> 8u));
}

} // namespace

std::uint32_t wordAddress() {
  return CRASH1_BIOS_PAD_READ_WORD_ADDRESS;
}

void publishPrimary(Core &core, std::uint16_t activeLowButtons) {
  constexpr std::uint32_t kDisconnectedPort = 0xFFFFu;
  const std::uint32_t primary = static_cast<std::uint32_t>(toBiosHalfword(activeLowButtons));
  core.mem_w32(wordAddress(), (primary << 16u) | kDisconnectedPort);
}

} // namespace crash1::bios_pad_input
