#include "core.h"
#include "crash1_frame_driver.h"
#include "crash1_runtime.h"
#include "game.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <memory>

namespace {

constexpr std::uint32_t kEntry = 0x80010000u;
constexpr std::uint32_t kLoop = 0x80010004u;
constexpr std::uint32_t kBoundary = 0x80010008u;

bool expect(bool condition, const char *message) {
  if (!condition) {
    std::fprintf(stderr, "%s\n", message);
  }
  return condition;
}

} // namespace

int main() {
  crash1::Crash1Runtime runtime;
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  auto &driver = static_cast<crash1::Crash1FrameDriver &>(*game->frameDriver);
  Core &core = game->core;

  std::array<std::uint32_t, 3> seen{};
  std::size_t calls = 0;
  const auto completed = driver.runGuestToBoundary(core, kEntry, [&](Core &current, std::uint32_t pc) {
    seen.at(calls++) = pc;
    if (calls == 1) {
      current.r[2] = 11u;
      return psx::cpu::ExecutionResult{
          psx::cpu::ExecutionExitReason::BudgetExhausted, kLoop, 564480u, "cycle budget exhausted"};
    }
    if (calls == 2) {
      current.r[2] = 12u;
      return psx::cpu::ExecutionResult{
          psx::cpu::ExecutionExitReason::BudgetExhausted, kLoop, 100u, "cycle budget exhausted"};
    }
    return psx::cpu::ExecutionResult{psx::cpu::ExecutionExitReason::FrameBoundary, kBoundary, 80u, "boundary"};
  });
  bool ok = expect(calls == 3 && seen == std::array{kEntry, kLoop, kLoop},
                   "Crash 1 turn did not resume both budget exits at their exact guest PCs");
  ok &= expect(completed.reason == psx::cpu::ExecutionExitReason::FrameBoundary && completed.guestPc == kBoundary &&
                   completed.cycles == 564660u && core.r[2] == 12u,
               "Crash 1 turn did not preserve guest state and cumulative cycles through its frame boundary");

  calls = 0;
  const auto stalled = driver.runGuestToBoundary(core, kEntry, [&](Core &, std::uint32_t) {
    ++calls;
    return psx::cpu::ExecutionResult{
        psx::cpu::ExecutionExitReason::BudgetExhausted, kEntry, 0u, "host dispatch budget exhausted"};
  });
  ok &= expect(calls == 1 && stalled.reason == psx::cpu::ExecutionExitReason::Fault && stalled.guestPc == kEntry &&
                   stalled.cycles == 0u,
               "Crash 1 turn repeated a zero-progress budget exit");

  calls = 0;
  const auto fault = driver.runGuestToBoundary(core, kEntry, [&](Core &, std::uint32_t pc) {
    ++calls;
    return psx::cpu::ExecutionResult{psx::cpu::ExecutionExitReason::Fault, pc, 17u, "synthetic fault"};
  });
  ok &= expect(calls == 1 && fault.reason == psx::cpu::ExecutionExitReason::Fault && fault.guestPc == kEntry &&
                   fault.cycles == 17u && fault.detail == "synthetic fault",
               "Crash 1 turn retried or swallowed a backend fault");
  return ok ? 0 : 1;
}
