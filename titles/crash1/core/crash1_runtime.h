#pragma once

#include "game_runtime.h"
#include "native_frame_loop_contract.h"

#include <memory>

namespace crash1 {

// Process-lifetime owner of Crash 1's executable-derived boot prefix and finite host frame loop.
// Crash 2/3 remain separate refusing runtimes until their own title addresses are measured.
class Crash1Runtime final : public GameRuntime {
public:
  Crash1Runtime();

  RenderCapabilities renderCapabilities() const override;
  void *createContext(Core &core) override;
  void destroyContext(void *context) override;
  void registerOverrides(Game &game) override;
  void bootInit(Core &core) override;
  const GuestProgramImage *guestProgramImage() const override;
  const PlatformHlePlan *platformHlePlan() const override;
  bool guestVramIsPicture(const Game &game) const override;
  std::unique_ptr<FrameDriver> createFrameDriver(Game &game) override;

  const crash::NativeFrameLoopContract &nativeFrameLoopContract() const;

private:
  static const GuestProgramImage programImage_;
  static const PlatformHlePlan platformPlan_;
};

} // namespace crash1
