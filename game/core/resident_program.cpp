#include "resident_program.h"

#include "game.h"
#include "game_runtime.h"
#include "native_frame_loop_contract.h"
#include "overlay_table.h"
#include "rec_decls.h"

#include <cstdio>
#include <memory>

void load_exe(const char *path, Core *core);

namespace crash {
namespace {

const RecompRegistry kResidentRecomp{
    .main_dispatch = main_dispatch,
    .rec_func_index = rec_func_index,
    .overlays = g_rec_overlays,
    .overlay_count = g_rec_overlay_count,
    .shard_set_override = shard_set_override,
    .ov_a00_set_override = nullptr,
    .ov_game_set_override = nullptr,
    .guestMemset_gen = nullptr,
};

struct DynamicBoundaryObservation {
  const ResidentProgram *program;
};

void observeDynamicBoundary(Core *core, std::uint64_t, std::uint32_t guestPc, void *user) {
  const auto &program = *static_cast<DynamicBoundaryObservation *>(user)->program;
  const std::uint32_t callerPc = core->pc;
  core->pc = guestPc;
  program.boundaryHandler(core);
  core->pc = callerPc;
}

} // namespace

void installResidentRuntime(GameRuntime &runtime) {
  psxport_install_game(runtime);
  psxport_install_recomp(&kResidentRecomp);
}

int runResidentProgram(const ResidentProgram &program) {
  if (program.executable == nullptr || *program.executable == '\0' || program.boundaryHandler == nullptr) {
    std::fprintf(stderr, "REFUSED: resident program request is incomplete\n");
    return 2;
  }
  if ((program.transitionBoundary == 0) != (program.transitionHandler == nullptr)) {
    std::fprintf(stderr, "REFUSED: resident transition boundary and handler must be supplied together\n");
    return 2;
  }
  if (program.transitionBoundary != 0 && program.transitionBoundary == program.boundary) {
    std::fprintf(stderr, "REFUSED: resident transition and final boundary cannot be the same address\n");
    return 2;
  }
  if (kResidentRecomp.rec_func_index(program.entry) < 0) {
    std::fprintf(stderr,
                 "REFUSED: %.*s generated substrate omits entry 0x%08X\n",
                 static_cast<int>(program.codeword.size()),
                 program.codeword.data(),
                 program.entry);
    return 2;
  }
  if (program.boundaryKind == ResidentBoundaryKind::GeneratedEntry &&
      kResidentRecomp.rec_func_index(program.boundary) < 0) {
    std::fprintf(stderr,
                 "REFUSED: %.*s generated substrate omits final generated boundary 0x%08X\n",
                 static_cast<int>(program.codeword.size()),
                 program.codeword.data(),
                 program.boundary);
    return 2;
  }
  if (program.transitionBoundary != 0 && kResidentRecomp.rec_func_index(program.transitionBoundary) < 0) {
    std::fprintf(stderr,
                 "REFUSED: %.*s generated substrate omits transition boundary 0x%08X\n",
                 static_cast<int>(program.codeword.size()),
                 program.codeword.data(),
                 program.transitionBoundary);
    return 2;
  }

  installResidentRuntime(program.runtime);
  auto game = std::make_unique<Game>();
  Core *core = &game->core;
  // Resident boundary products bypass psxport's dc_boot_init, so they must install and require the
  // same native-frame-loop VSync contract explicitly before any generated guest dispatch.
  crash::initializeNativeFrameLoopContract(*game);
  DynamicBoundaryObservation dynamicBoundary{&program};
  load_exe(program.executable, core);
  if (program.transitionBoundary != 0) {
    kResidentRecomp.shard_set_override(program.transitionBoundary, program.transitionHandler);
  }
  if (program.boundaryKind == ResidentBoundaryKind::DynamicDispatch) {
    if (!core->pcObserver.arm(&program.boundary, 1, observeDynamicBoundary, &dynamicBoundary)) {
      std::fprintf(stderr,
                   "REFUSED: %.*s could not arm dynamic-dispatch boundary 0x%08X\n",
                   static_cast<int>(program.codeword.size()),
                   program.codeword.data(),
                   program.boundary);
      return 2;
    }
  } else {
    kResidentRecomp.shard_set_override(program.boundary, program.boundaryHandler);
  }
  kResidentRecomp.main_dispatch(core, program.entry);
  std::fprintf(stderr,
               "FAIL: %.*s entry 0x%08X returned without reaching boundary 0x%08X\n",
               static_cast<int>(program.codeword.size()),
               program.codeword.data(),
               program.entry,
               program.boundary);
  return 1;
}

} // namespace crash
