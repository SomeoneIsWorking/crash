#include "dynarec_dispatch.h"
#include "core.h"
#include "crash1_runtime.h"
#include "execution_control.h"
#include "game.h"
#include "lightrec_executor.h"

#include <cstdio>
#include <memory>

namespace {

constexpr std::uint32_t kEntry = 0x80010000u;
constexpr std::uint32_t kReturn = 0x80010100u;
constexpr std::uint32_t kTurn = 0x80010200u;
constexpr std::uint32_t kFrame = 0x80010300u;
unsigned int nativeCalls = 0;

void addNativeResult(Core *core) {
  ++nativeCalls;
  const auto original = crash::dynarec::callOriginal(*core, kEntry);
  crash::dynarec::requireGuestReturn(original, "synthetic Crash original call");
  core->r[2] += 10u;
}

void finishFrame(Core *core) {
  psx::cpu::requestExecutionExit(*core, psx::cpu::ExecutionExitReason::FrameBoundary);
}

bool runFunction(Core &core, std::uint32_t expected) {
  core.r[31] = kReturn;
  const auto result = crash::dynarec::callGuest(core, kEntry);
  if (result.returned() && result.guestPc == kReturn && core.r[2] == expected) {
    return true;
  }
  std::fprintf(stderr,
               "Crash dispatch: exit=%s pc=0x%08X value=%u expected=%u\n",
               psx::cpu::executionExitName(result.reason),
               result.guestPc,
               core.r[2],
               expected);
  return false;
}

} // namespace

int main() {
  crash1::Crash1Runtime runtime;
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  Core &core = game->core;
  if (crash::dynarec::installOverride(core, kEntry, "unmapped fixture", addNativeResult)) {
    std::fprintf(stderr, "Crash accepted an override without an active image\n");
    return 1;
  }
  core.imageCatalog().activate(
      "synthetic Crash dispatcher fixture", {kEntry & 0x1FFFFFFFu, (kFrame & 0x1FFFFFFFu) + 4u}, 1u);
  core.mem_w32(kEntry, 0x2402001Fu);      // addiu v0, zero, 31
  core.mem_w32(kEntry + 4u, 0x03E00008u); // jr ra
  core.mem_w32(kEntry + 8u, 0u);          // delay-slot nop
  if (!runFunction(core, 31u)) {
    return 1;
  }
  if (!crash::dynarec::installOverride(core, kEntry, "synthetic native augmentation", addNativeResult) ||
      !runFunction(core, 41u)) {
    return 1;
  }
  const auto before = core.lightrecExecutor().counters();
  core.mem_w32(kEntry, 0x24020029u); // replace guest result with 41
  if (!runFunction(core, 51u)) {
    return 1;
  }
  core.mem_w32(kTurn, 0x0C000000u | ((kEntry >> 2u) & 0x03FFFFFFu)); // jal native/original augmentation
  core.mem_w32(kTurn + 4u, 0u);
  core.mem_w32(kTurn + 8u, 0x08000000u | ((kFrame >> 2u) & 0x03FFFFFFu)); // j frame owner
  core.mem_w32(kTurn + 12u, 0u);
  if (!crash::dynarec::installOverride(core, kFrame, "synthetic frame completion", finishFrame)) {
    return 1;
  }
  const auto frame = crash::dynarec::executeTurn(core, kTurn);
  if (frame.reason != psx::cpu::ExecutionExitReason::FrameBoundary || frame.guestPc != kFrame || core.r[2] != 51u) {
    std::fprintf(stderr, "Crash turn failed to service native calls and reach its requested frame exit\n");
    return 1;
  }
  const auto &after = core.lightrecExecutor().counters();
  if (nativeCalls != 3u || after.translatedBlocks == 0u || after.executedBlocks == 0u ||
      after.executedInstructions == 0u || after.invalidations <= before.invalidations || after.fallback.calls != 0u ||
      after.fallback.instructions != 0u) {
    std::fprintf(stderr,
                 "Crash dispatch did not prove native/original/JIT execution and invalidation without fallback\n");
    return 1;
  }
  core.lightrecExecutor().reportFallbackTelemetry("Crash synthetic dispatcher");
  return 0;
}
