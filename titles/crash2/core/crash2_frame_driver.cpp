#include "crash2_frame_driver.h"

#ifndef CRASH_TITLE_VSYNC_ENTRY
#error "CRASH_TITLE_VSYNC_ENTRY must come from titles/crash2/executable.json"
#endif
#ifndef CRASH_TITLE_VSYNC_END
#error "CRASH_TITLE_VSYNC_END must come from titles/crash2/executable.json"
#endif

namespace crash2 {
namespace {

constexpr crash::NativeFrameLoopContract kContract{
    .codeword = "SCUS-94154",
    .guestVSync = {CRASH_TITLE_VSYNC_ENTRY, CRASH_TITLE_VSYNC_END},
    .state = crash::NativeFrameLoopState::Missing,
    .refusal = "Crash 2 has no shipping product or measured frame boundary.",
};
static_assert(kContract.guestVSync.valid());

} // namespace

const crash::NativeFrameLoopContract &Crash2FrameDriver::contract() {
  return kContract;
}

} // namespace crash2
