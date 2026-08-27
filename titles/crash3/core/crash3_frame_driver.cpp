#include "crash3_frame_driver.h"

#ifndef CRASH_TITLE_VSYNC_ENTRY
#error "CRASH_TITLE_VSYNC_ENTRY must come from titles/crash3/executable.json"
#endif
#ifndef CRASH_TITLE_VSYNC_END
#error "CRASH_TITLE_VSYNC_END must come from titles/crash3/executable.json"
#endif

namespace crash3 {
namespace {

constexpr crash::NativeFrameLoopContract kContract{
    .codeword = "SCUS-94244",
    .guestVSync = {CRASH_TITLE_VSYNC_ENTRY, CRASH_TITLE_VSYNC_END},
    .state = crash::NativeFrameLoopState::Missing,
    .refusal = "Crash 3 has no shipping product or measured frame boundary.",
};
static_assert(kContract.guestVSync.valid());

} // namespace

const crash::NativeFrameLoopContract &Crash3FrameDriver::contract() {
  return kContract;
}

} // namespace crash3
