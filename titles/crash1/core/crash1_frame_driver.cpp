#include "crash1_frame_driver.h"

#include "core.h"
#include "emulated_time.h"
#include "field_rate.h"
#include "game.h"
#include "recomp_iface.h"

#include <cstdlib>
#include <lucent/log.h>

#ifndef CRASH_TITLE_VSYNC_ENTRY
#error "CRASH_TITLE_VSYNC_ENTRY must come from titles/crash1/executable.json"
#endif
#ifndef CRASH_TITLE_VSYNC_END
#error "CRASH_TITLE_VSYNC_END must come from titles/crash1/executable.json"
#endif
#ifndef CRASH1_CORE_LOOP_ENTRY
#error "CRASH1 frame facts must come from titles/crash1/executable.json"
#endif

namespace crash1 {
namespace {

constexpr std::uint32_t kRootCounterSpec = 0xF2000002u;
constexpr std::uint32_t kRootCounterTarget = 0x1000u;
constexpr std::uint64_t kRootCounterCpuTicks = 8ull * kRootCounterTarget;

constexpr crash::NativeFrameLoopContract kContract{
    .codeword = "SCUS-94900",
    .guestVSync = {CRASH_TITLE_VSYNC_ENTRY, CRASH_TITLE_VSYNC_END},
    .state = crash::NativeFrameLoopState::FiniteBootSeamOnly,
    .refusal = "candidate frame step is not ready until a real product frame returns",
};

constexpr Crash1FrameProgram kProgram{
    .coreLoop = {CRASH1_CORE_LOOP_ENTRY, CRASH1_CORE_LOOP_END},
    .iteration = {CRASH1_FRAME_ITERATION_ENTRY, CRASH1_FRAME_ITERATION_END},
    .transition = {CRASH1_FRAME_TRANSITION_ENTRY, CRASH1_FRAME_TRANSITION_END},
    .gpuUpdate = {CRASH1_GPU_UPDATE_ENTRY, CRASH1_GPU_UPDATE_END},
    .firstVSync = CRASH1_FIRST_VSYNC,
    .afterFirstVSync = CRASH1_AFTER_FIRST_VSYNC,
    .secondVSync = CRASH1_SECOND_VSYNC,
    .afterVSync = CRASH1_AFTER_VSYNC,
    .doneAddress = CRASH1_DONE_ADDRESS,
    .ticksElapsedAddress = CRASH1_TICKS_ELAPSED_ADDRESS,
    .displayContextAddress = CRASH1_DISPLAY_CONTEXT_ADDRESS,
    .rootCounterIncrement = CRASH1_ROOT_COUNTER_INCREMENT,
    .setRootCounter = CRASH1_SET_ROOT_COUNTER,
    .startRootCounter = CRASH1_START_ROOT_COUNTER,
    .stopRootCounter = CRASH1_STOP_ROOT_COUNTER,
};

static_assert(kContract.guestVSync.valid());
static_assert(kProgram.coreLoop.valid() && kProgram.iteration.valid() && kProgram.transition.valid() &&
              kProgram.gpuUpdate.valid());
static_assert(kProgram.coreLoop.contains(kProgram.iteration.begin));
static_assert(kProgram.coreLoop.contains(kProgram.transition.begin));
static_assert(kProgram.gpuUpdate.contains(kProgram.firstVSync));
static_assert(kProgram.gpuUpdate.contains(kProgram.afterFirstVSync));
static_assert(kProgram.gpuUpdate.contains(kProgram.secondVSync));
static_assert(kProgram.gpuUpdate.contains(kProgram.afterVSync));

const RecompRegistry &requireRecompiledProgram() {
  const RecompRegistry *registry = psxport_recomp();
  if (registry == nullptr || registry->main_dispatch == nullptr || registry->rec_func_index == nullptr ||
      registry->shard_set_override == nullptr) {
    lucent::error("crash1-frame", "Crash 1 has no installed resident recompiler registry");
    std::abort();
  }
  return *registry;
}

void requireEntry(const RecompRegistry &registry, std::uint32_t address, const char *owner) {
  if (registry.rec_func_index(address) < 0) {
    lucent::error("crash1-frame", "{} entry 0x{:08X} is absent from the generated substrate", owner, address);
    std::abort();
  }
}

} // namespace

Crash1FrameDriver::Crash1FrameDriver(Game &game) : game_(game) {}

const crash::NativeFrameLoopContract &Crash1FrameDriver::contract() {
  return kContract;
}

const Crash1FrameProgram &Crash1FrameDriver::program() {
  return kProgram;
}

Crash1FrameDriver &Crash1FrameDriver::from(Core &core) {
  if (core.game == nullptr || core.game->frameDriver == nullptr) {
    lucent::error("crash1-frame", "Crash 1 frame override has no bound FrameDriver");
    std::abort();
  }
  return static_cast<Crash1FrameDriver &>(*core.game->frameDriver);
}

void Crash1FrameDriver::installOverrides(Game &) {
  const RecompRegistry &registry = requireRecompiledProgram();
  const struct Binding {
    std::uint32_t address;
    RecOverrideFn function;
    const char *owner;
  } bindings[]{
      {kProgram.transition.begin, finishFrameIteration, "CoreLoop transition"},
      {kProgram.firstVSync, deliverFirstDisplayField, "first GpuUpdate VSync"},
      {kProgram.secondVSync, deliverSecondDisplayField, "second GpuUpdate VSync"},
      {kProgram.setRootCounter, setRootCounterSuper, "SetRCnt"},
      {kProgram.startRootCounter, startRootCounterSuper, "StartRCnt"},
      {kProgram.stopRootCounter, stopRootCounterSuper, "StopRCnt"},
  };
  for (const Binding &binding : bindings) {
    requireEntry(registry, binding.address, binding.owner);
    registry.shard_set_override(binding.address, binding.function);
  }
  requireEntry(registry, kProgram.coreLoop.begin, "CoreLoop");
  requireEntry(registry, kProgram.iteration.begin, "CoreLoop iteration");
  requireEntry(registry, kProgram.afterFirstVSync, "first VSync continuation");
  requireEntry(registry, kProgram.afterVSync, "GpuUpdate post-VSync super");
  requireEntry(registry, kProgram.rootCounterIncrement, "root-counter event super");
}

void Crash1FrameDriver::runRecompiledSuper(Core &core, std::uint32_t address, void (*overrideFn)(Core *)) {
  const RecompRegistry &registry = requireRecompiledProgram();
  registry.shard_set_override(address, nullptr);
  registry.main_dispatch(&core, address);
  registry.shard_set_override(address, overrideFn);
}

void Crash1FrameDriver::resetRootCounterClock(const Core &core) {
  nextRootCounterTick_ = core.game->timing.emulatedCpuTicks() + kRootCounterCpuTicks;
}

void Crash1FrameDriver::serviceRootCounter(Core &core, std::uint64_t throughCpuTick) {
  if (!rootCounterConfigured_ || !rootCounterRunning_) {
    return;
  }
  const RecompRegistry &registry = requireRecompiledProgram();
  while (nextRootCounterTick_ <= throughCpuTick) {
    const R3000 interrupted = static_cast<const R3000 &>(core);
    registry.main_dispatch(&core, kProgram.rootCounterIncrement);
    static_cast<R3000 &>(core) = interrupted;
    nextRootCounterTick_ += kRootCounterCpuTicks;
  }
}

void Crash1FrameDriver::setRootCounterSuper(Core *core) {
  Crash1FrameDriver &driver = from(*core);
  const std::uint32_t spec = core->r[4];
  const std::uint32_t target = core->r[5];
  driver.runRecompiledSuper(*core, kProgram.setRootCounter, setRootCounterSuper);
  if (spec == kRootCounterSpec && target == kRootCounterTarget) {
    driver.rootCounterConfigured_ = true;
    driver.resetRootCounterClock(*core);
  }
}

void Crash1FrameDriver::startRootCounterSuper(Core *core) {
  Crash1FrameDriver &driver = from(*core);
  const std::uint32_t spec = core->r[4];
  driver.runRecompiledSuper(*core, kProgram.startRootCounter, startRootCounterSuper);
  if (spec == kRootCounterSpec) {
    if (!driver.rootCounterConfigured_) {
      lucent::error("crash1-frame", "StartRCnt(CNT2) preceded the measured SetRCnt(CNT2,0x1000) setup");
      std::abort();
    }
    driver.rootCounterRunning_ = true;
    driver.resetRootCounterClock(*core);
  }
}

void Crash1FrameDriver::stopRootCounterSuper(Core *core) {
  Crash1FrameDriver &driver = from(*core);
  const std::uint32_t spec = core->r[4];
  if (spec == kRootCounterSpec) {
    driver.serviceRootCounter(*core, core->game->timing.emulatedCpuTicks());
    driver.rootCounterRunning_ = false;
  }
  driver.runRecompiledSuper(*core, kProgram.stopRootCounter, stopRootCounterSuper);
}

void Crash1FrameDriver::deliverDisplayField(Core &core, std::uint32_t continuation) {
  if (!rootCounterConfigured_ || !rootCounterRunning_) {
    lucent::error("crash1-frame", "GpuUpdate reached its display wait before Crash 1 CNT2 was configured and started");
    std::abort();
  }
  if (deliveredFields_ >= 2) {
    lucent::error("crash1-frame", "one retail GpuUpdate requested more than its measured two display fields");
    std::abort();
  }
  if (deliveredFields_ == 0) {
    waitBaseCpuTick_ = game_.timing.emulatedCpuTicks();
  }
  ++deliveredFields_;
  const std::uint64_t displayBoundary =
      waitBaseCpuTick_ + display_field_cpu_ticks(deliveredFields_, 1u, FIELD_RATE_NTSC_MILLIHZ);
  serviceRootCounter(core, displayBoundary);
  if (game_.diff_mode) {
    game_.spu_audio.frameLogic();
  } else {
    game_.spu_audio.frame();
  }
  core.r[31] = continuation;
  requireRecompiledProgram().main_dispatch(&core, continuation);
}

void Crash1FrameDriver::deliverFirstDisplayField(Core *core) {
  from(*core).deliverDisplayField(*core, kProgram.afterFirstVSync);
}

void Crash1FrameDriver::deliverSecondDisplayField(Core *core) {
  from(*core).deliverDisplayField(*core, kProgram.afterVSync);
}

void Crash1FrameDriver::finishFrameIteration(Core *core) {
  if (core->r[31] != kProgram.transition.begin) {
    lucent::error("crash1-frame",
                  "CoreLoop transition reached with ra=0x{:08X}; expected retail GpuUpdate return 0x{:08X}",
                  core->r[31],
                  kProgram.transition.begin);
    std::abort();
  }
}

void Crash1FrameDriver::stepFrame(Core &core, std::uint32_t frame) {
  if (&core != &game_.core || core.game != &game_) {
    lucent::error("crash1-frame", "Crash1FrameDriver was asked to step a different Game/Core");
    std::abort();
  }
  if (frame != completedFrames_) {
    lucent::error(
        "crash1-frame", "non-sequential frame {} requested after {} completed frame(s)", frame, completedFrames_);
    std::abort();
  }

  installOverrides(game_);
  serviceRootCounter(core, game_.timing.emulatedCpuTicks());
  game_.timing.logicFrame = frame;
  game_.timing.frameTick();
  core.rsub.otAttr.beginLogicFrame(frame);
  game_.pad.serviceFrame();
  deliveredFields_ = 0;

  const RecompRegistry &registry = requireRecompiledProgram();
  if (!enteredCoreLoop_) {
    core.r[4] = 25u;
    registry.main_dispatch(&core, kProgram.coreLoop.begin);
    enteredCoreLoop_ = true;
  } else {
    registry.main_dispatch(&core, kProgram.iteration.begin);
  }

  if (core.mem_r32(kProgram.doneAddress) != 0) {
    lucent::error("crash1-frame",
                  "Crash 1 requested CoreLoop exit after frame {}; its measured two-GpuUpdate drain/NSKill "
                  "transition is not yet extracted into host turns",
                  frame);
    std::abort();
  }
  if (deliveredFields_ == 0 || deliveredFields_ > 2) {
    lucent::error("crash1-frame", "frame {} returned without the measured one-or-two GpuUpdate display waits", frame);
    std::abort();
  }

  game_.presentation.commit(&core, static_cast<int>(deliveredFields_), game_.temporalPresentation.get());
  ++completedFrames_;
}

} // namespace crash1
