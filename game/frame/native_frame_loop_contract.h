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
  // The libetc field counter this title's VSync body returns for a NEGATIVE argument. That call is a
  // query for the elapsed field count, not a frame wait, so the framework answers it from here and
  // does not advance a field. Measured per title by disassembling the body's `bgez $a0` arm: all three
  // load one global and return it (Crash 1 0x800549F0, Crash 2 0x8005DC98, Crash 3 0x8005F384, each
  // also the counter the body increments past its callback wait). Zero means undeclared, and the
  // framework then refuses the query by name rather than fabricating a count.
  std::uint32_t vsyncQueryCounter;
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
