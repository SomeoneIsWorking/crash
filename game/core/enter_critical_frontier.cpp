#include "enter_critical_frontier.h"

#include "core.h"
#include "game.h"

#include <cstdio>

namespace crash {

EnterCriticalResult runEnterCriticalFrontier(Core &core, std::string_view codeword, RecOverrideFn generatedBody) {
  constexpr std::uint32_t kLoadEnterCriticalSelector = 0x24040001u;
  constexpr std::uint32_t kSyscallZero = 0x0000000Cu;
  if (generatedBody == nullptr) {
    return {.detail = "generated EnterCriticalSection body is absent"};
  }
  if (core.mem_r32(core.pc) != kLoadEnterCriticalSelector || core.mem_r32(core.pc + 4) != kSyscallZero) {
    char detail[192]{};
    std::snprintf(detail,
                  sizeof(detail),
                  "requested syscall boundary 0x%08X is not %.*s's measured "
                  "addiu-a0-1/syscall-0 wrapper",
                  core.pc,
                  static_cast<int>(codeword.size()),
                  codeword.data());
    return {.detail = detail};
  }

  EnterCriticalObservation observation{
      .boundary = core.pc,
      .irqBefore = core.game->hle.irq_enabled,
      .statusBefore = core.cop0[12],
  };
  generatedBody(&core);
  observation.selector = core.r[4];
  observation.returnValue = core.r[2];
  observation.irqAfter = core.game->hle.irq_enabled;
  observation.statusAfter = core.cop0[12];

  const std::uint32_t expectedStatus = observation.statusBefore & ~1u;
  if (observation.selector != 1u || observation.returnValue != 1u || observation.irqBefore != 1 ||
      observation.irqAfter != 0 || observation.statusAfter != expectedStatus) {
    char detail[256]{};
    std::snprintf(detail,
                  sizeof(detail),
                  "EnterCriticalSection transition disagrees: selector=%u return=%u irq=%d->%d "
                  "status=0x%08X->0x%08X",
                  observation.selector,
                  observation.returnValue,
                  observation.irqBefore,
                  observation.irqAfter,
                  observation.statusBefore,
                  observation.statusAfter);
    return {.observation = observation, .detail = detail};
  }
  return {.accepted = true,
          .observation = observation,
          .detail = "generated wrapper reached the measured EnterCriticalSection frontier"};
}

} // namespace crash
