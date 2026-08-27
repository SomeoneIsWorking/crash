#include "crash1_runtime.h"
#include "recomp_iface.h"
#include "title_runtime_contract.h"

namespace {

void unusedDispatch(Core *, std::uint32_t) {}
int residentEntry(std::uint32_t) {
  return 0;
}
void acceptOverride(std::uint32_t, RecOverrideFn) {}

const RecompRegistry kResidentRegistry{
    .main_dispatch = unusedDispatch,
    .rec_func_index = residentEntry,
    .overlays = nullptr,
    .overlay_count = 0,
    .shard_set_override = acceptOverride,
    .ov_a00_set_override = nullptr,
    .ov_game_set_override = nullptr,
    .guestMemset_gen = nullptr,
};

} // namespace

int main() {
  psxport_install_recomp(&kResidentRegistry);
  return verifyTitleRuntimeContract<crash1::Crash1Runtime>(
      "Crash1Runtime", true, crash::NativeFrameLoopState::FiniteBootSeamOnly);
}
