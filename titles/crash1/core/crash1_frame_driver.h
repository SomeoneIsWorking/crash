#pragma once

#include "game_runtime.h"
#include "native_frame_loop_contract.h"

#include <cstdint>

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
  explicit Crash1FrameDriver(Game &game);

  void stepFrame(Core &core, std::uint32_t frame) override;

  static const crash::NativeFrameLoopContract &contract();
  static const Crash1FrameProgram &program();
  static void installOverrides(Game &game);

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
