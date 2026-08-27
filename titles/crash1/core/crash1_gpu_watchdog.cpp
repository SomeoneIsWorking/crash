#include "crash1_gpu_watchdog.h"

#include "core.h"
#include "game.h"
#include "recomp_iface.h"

#include <cstdlib>
#include <lucent/log.h>

#ifndef CRASH1_GPU_WATCHDOG_START_ENTRY
#error "CRASH1 GPU watchdog facts must come from titles/crash1/executable.json"
#endif
#ifndef CRASH1_GPU_WATCHDOG_START_END
#error "CRASH1 GPU watchdog facts must come from titles/crash1/executable.json"
#endif
#ifndef CRASH1_GPU_WATCHDOG_CHECK_ENTRY
#error "CRASH1 GPU watchdog facts must come from titles/crash1/executable.json"
#endif
#ifndef CRASH1_GPU_WATCHDOG_CHECK_END
#error "CRASH1 GPU watchdog facts must come from titles/crash1/executable.json"
#endif

namespace crash1::gpu_watchdog {
namespace {

constexpr Program kProgram{
    .start = {CRASH1_GPU_WATCHDOG_START_ENTRY, CRASH1_GPU_WATCHDOG_START_END},
    .check = {CRASH1_GPU_WATCHDOG_CHECK_ENTRY, CRASH1_GPU_WATCHDOG_CHECK_END},
};

constexpr std::uint32_t kDeadline = 0x80054B84u;
constexpr std::uint32_t kPollCount = 0x80054B88u;
constexpr std::uint32_t kQueueRead = 0x80054B74u;
constexpr std::uint32_t kQueueWrite = 0x80054B70u;
constexpr std::uint32_t kGpuGp0Pointer = 0x80054B40u;
constexpr std::uint32_t kGpuStatusPointer = 0x80054B44u;
constexpr std::uint32_t kGpuGp1Pointer = 0x80054B4Cu;
constexpr std::uint32_t kGpuDmaControlPointer = 0x80054B5Cu;
constexpr std::uint32_t kLastCallback = 0x80054B60u;
constexpr std::uint32_t kLastPayload = 0x80054B64u;
constexpr std::uint32_t kLastArgument = 0x80054B68u;
constexpr std::uint32_t kCriticalToken = 0x80054B80u;
constexpr std::uint32_t kDiagnosticFormat = 0x80011350u;
constexpr std::uint32_t kCallbackFormat = 0x80011384u;
constexpr std::uint32_t kLog = 0x8003D730u;
constexpr std::uint32_t kCritical = 0x8003E870u;
constexpr std::uint32_t kDisplayTimeoutFields = 240u;
constexpr std::uint32_t kPollLimit = 0xF0000u;

static_assert(kProgram.start.valid() && kProgram.check.valid());

const RecompRegistry &requireRegistry() {
  const RecompRegistry *const registry = psxport_recomp();
  if (registry == nullptr || registry->main_dispatch == nullptr || registry->rec_func_index == nullptr ||
      registry->shard_set_override == nullptr) {
    lucent::error("crash1-gpu-watchdog", "GPU watchdog owner has no installed resident recompiler registry");
    std::abort();
  }
  return *registry;
}

void startOverride(Core *core) {
  start(*core);
}

void checkOverride(Core *core) {
  check(*core, requireRegistry().main_dispatch);
}

void dispatchAt(Core &core, MainDispatch dispatch, std::uint32_t address, std::uint32_t continuation) {
  core.r[31] = continuation;
  dispatch(&core, address);
}

} // namespace

const Program &program() {
  return kProgram;
}

void registerOverrides() {
  const RecompRegistry &registry = requireRegistry();
  const struct Binding {
    std::uint32_t address;
    RecOverrideFn function;
  } bindings[]{
      {kProgram.start.begin, startOverride},
      {kProgram.check.begin, checkOverride},
  };
  for (const Binding &binding : bindings) {
    if (registry.rec_func_index(binding.address) < 0) {
      lucent::error("crash1-gpu-watchdog",
                    "measured GPU watchdog entry 0x{:08X} is absent from the generated substrate",
                    binding.address);
      std::abort();
    }
    registry.shard_set_override(binding.address, binding.function);
  }
}

void start(Core &core) {
  // Retail uses VSync(-1) only to read the libetc field count. Timing::frameTick is the native loop's
  // sole owner of the same compatibility counter, so consume it directly without entering VSync.
  core.mem_w32(kDeadline, core.game->timing.vblank + kDisplayTimeoutFields);
  core.mem_w32(kPollCount, 0u);
  core.r[2] = 0u;
}

void check(Core &core, MainDispatch dispatch) {
  const std::uint32_t pollCount = core.mem_r32(kPollCount);
  core.mem_w32(kPollCount, pollCount + 1u);
  const bool displayExpired =
      static_cast<std::int32_t>(core.mem_r32(kDeadline)) < static_cast<std::int32_t>(core.game->timing.vblank);
  if (!displayExpired && pollCount <= kPollLimit) {
    core.r[2] = 0u;
    return;
  }

  if (dispatch == nullptr) {
    lucent::error("crash1-gpu-watchdog", "GPU watchdog timeout owner received a null main dispatcher");
    std::abort();
  }

  const std::uint32_t incomingStack = core.r[29];
  const std::uint32_t incomingReturn = core.r[31];
  core.r[29] -= 32u;
  core.mem_w32(core.r[29] + 24u, incomingReturn);

  core.r[4] = kDiagnosticFormat;
  core.r[5] = (core.mem_r32(kQueueWrite) - core.mem_r32(kQueueRead)) & 63u;
  core.r[6] = core.mem_r32(core.mem_r32(kGpuGp0Pointer));
  core.r[7] = core.mem_r32(core.mem_r32(kGpuGp1Pointer));
  core.mem_w32(core.r[29] + 16u, core.mem_r32(core.mem_r32(kGpuStatusPointer)));
  dispatchAt(core, dispatch, kLog, 0x80042938u);

  core.r[4] = kCallbackFormat;
  core.r[5] = core.mem_r32(kLastCallback);
  core.r[6] = core.mem_r32(kLastPayload);
  core.r[7] = core.mem_r32(kLastArgument);
  dispatchAt(core, dispatch, kLog, 0x80042964u);

  core.r[4] = 0u;
  dispatchAt(core, dispatch, kCritical, 0x8004296Cu);
  core.mem_w32(kCriticalToken, core.r[2]);
  core.mem_w32(kQueueRead, 0u);
  core.mem_w32(kQueueWrite, 0u);
  core.mem_w32(core.mem_r32(kGpuGp1Pointer), 0x401u);
  const std::uint32_t dmaControl = core.mem_r32(kGpuDmaControlPointer);
  core.mem_w32(dmaControl, core.mem_r32(dmaControl) | 0x800u);
  const std::uint32_t gpuGp0 = core.mem_r32(kGpuGp0Pointer);
  core.mem_w32(gpuGp0, 0x02000000u);
  core.mem_w32(gpuGp0, 0x01000000u);
  core.r[4] = core.mem_r32(kCriticalToken);
  dispatchAt(core, dispatch, kCritical, 0x800429E8u);

  core.r[2] = 0xFFFFFFFFu;
  core.r[31] = core.mem_r32(core.r[29] + 24u);
  core.r[29] = incomingStack;
}

} // namespace crash1::gpu_watchdog
