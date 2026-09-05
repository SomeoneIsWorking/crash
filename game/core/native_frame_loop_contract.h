#pragma once

#include <cstdint>
#include <string_view>

struct PlatformHlePlan;
class Game;

namespace crash {

struct GuestFunctionRange {
  std::uint32_t begin;
  std::uint32_t end;

  [[nodiscard]] constexpr bool contains(std::uint32_t address) const {
    return address >= begin && address < end;
  }

  [[nodiscard]] constexpr bool valid() const {
    return begin != 0 && begin < end;
  }
};

enum class NativeFrameLoopState {
  Ready,
  FiniteBootSeamOnly,
  Missing,
};

// Title-owned facts at the host/guest frame boundary. A non-null FrameDriver establishes that the
// host owns every attempted frame step. State says whether that driver can honestly advance one.
struct NativeFrameLoopContract {
  std::string_view codeword;
  GuestFunctionRange guestVSync;
  NativeFrameLoopState state;
  std::string_view refusal;

  [[nodiscard]] constexpr bool canStepFrame() const {
    return state == NativeFrameLoopState::Ready;
  }
};

// The current Crash drivers call this rather than entering an unmeasured guest frame loop. It is a
// production guard, not a test stub: reaching it means a host frame was requested beyond the title's
// recorded RE frontier.
[[noreturn]] void abortUnprovenFrameStep(const NativeFrameLoopContract &contract, std::uint32_t frame);

// Construct the sole allowed platform plan for a title-owned frame driver. Keeping this mapping
// here prevents each title runtime from independently interpreting the VSync contract.
[[nodiscard]] PlatformHlePlan makeNativeFramePlatformPlan(const NativeFrameLoopContract &contract);

// Direct product routes that do not enter psxport's standard bootstrap call this exact seam before
// guest execution. It installs and verifies the framework-owned typed VSync boundary.
void initializeNativeFrameLoopContract(Game &game);

} // namespace crash
