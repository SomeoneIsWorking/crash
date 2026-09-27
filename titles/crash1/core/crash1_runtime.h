#pragma once

#include "crash1_widescreen.h"
#include "game_runtime.h"
#include "gpu_vk.h"
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
  const GuestWidescreenProjection *guestWidescreenProjection() const override;

  const crash::NativeFrameLoopContract &nativeFrameLoopContract() const;

  // The measured projection owner this runtime hands the framework's latch. Exposed for the tests
  // that drive the publication sites, and for nothing else: the shipping path reaches it through
  // `guestWidescreenProjection()` like any other title policy.
  Crash1Widescreen &widescreen() {
    return widescreen_;
  }
  const Crash1Widescreen &widescreen() const {
    return widescreen_;
  }

private:
  static const GuestProgramImage programImage_;
  static const PlatformHlePlan platformPlan_;
  Crash1Widescreen widescreen_{&gpu_vk_latch_guest_projection};
};

} // namespace crash1
