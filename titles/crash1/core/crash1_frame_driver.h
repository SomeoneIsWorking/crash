#pragma once

#include "execution_exit.h"
#include "game_runtime.h"
#include "native_frame_loop_contract.h"

#include <cstdint>
#include <functional>

class Game;

namespace crash1 {

struct Crash1FrameProgram {
  crash::GuestFunctionRange coreLoop;
  crash::GuestFunctionRange iteration;
  crash::GuestFunctionRange transition;
  crash::GuestFunctionRange gpuUpdate;
  std::uint32_t firstVSync;
  std::uint32_t afterFirstVSync;
  std::uint32_t secondVSync;
  std::uint32_t afterVSync;
  std::uint32_t doneAddress;
  std::uint32_t ticksElapsedAddress;
  std::uint32_t displayContextAddress;
  std::uint32_t rootCounterIncrement;
  std::uint32_t setRootCounter;
  std::uint32_t startRootCounter;
  std::uint32_t stopRootCounter;
};

class Crash1FrameDriver final : public FrameDriver {
public:
  using ExecuteSlice = std::function<psx::cpu::ExecutionResult(Core &, std::uint32_t)>;

  explicit Crash1FrameDriver(Game &game);

  void stepFrame(Core &core, std::uint32_t frame) override;
  psx::cpu::ExecutionResult runGuestToBoundary(Core &core, std::uint32_t entry, const ExecuteSlice &execute);

  static const crash::NativeFrameLoopContract &contract();
  static const Crash1FrameProgram &program();
  static void installOverrides(Game &game);

  // Whether a typed FrameBoundary was raised from one of this title's MEASURED display waits, and
  // where the guest continues. Two provenances exist and both are the title's own: the GUEST's `jal` to
  // the libetc VSync leaf inside GpuUpdate, which leaves the standing architectural PC and `$r31` at the
  // instruction AFTER the `jal`; and the CoreLoop TRANSITION override, which stamps the VSync leaf's
  // entry as the boundary's provenance and leaves `$r31` at the transition's own return address. Both
  // continuations are this title's, so a boundary that matches neither is a refusal, not a guess.
  [[nodiscard]] static constexpr bool isMeasuredFrameBoundary(std::uint32_t guestPc,
                                                              std::uint32_t returnAddress,
                                                              std::uint32_t vsyncLeaf,
                                                              std::uint32_t afterFirst,
                                                              std::uint32_t afterSecond,
                                                              std::uint32_t transition) noexcept {
    const bool fromGuestVsync = (guestPc == afterFirst && returnAddress == afterFirst) ||
                                (guestPc == afterSecond && returnAddress == afterSecond);
    const bool fromTransitionOverride = guestPc == vsyncLeaf && returnAddress == transition;
    return fromGuestVsync || fromTransitionOverride;
  }

private:
  static void finishFrameIteration(Core *core);
  static void setRootCounterSuper(Core *core);
  static void startRootCounterSuper(Core *core);
  static void stopRootCounterSuper(Core *core);
  static Crash1FrameDriver &from(Core &core);

  void deliverDisplayField(Core &core);
  void serviceRootCounter(Core &core, std::uint64_t throughCpuTick);
  void resetRootCounterClock(const Core &core);
  void callOriginal(Core &core, std::uint32_t address);

  Game &game_;
  std::uint32_t completedFrames_{};
  std::uint32_t deliveredFields_{};
  std::uint64_t waitBaseCpuTick_{};
  std::uint64_t nextRootCounterTick_{};
  bool enteredCoreLoop_{};
  bool rootCounterConfigured_{};
  bool rootCounterRunning_{};
  bool frameCompleted_{};
};

} // namespace crash1
