#include "resident_program.h"

#include "game.h"
#include "game_runtime.h"
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

} // namespace

int runResidentProgram(const ResidentProgram &program) {
  if (program.executable == nullptr || *program.executable == '\0' || program.boundaryHandler == nullptr) {
    std::fprintf(stderr, "REFUSED: resident program request is incomplete\n");
    return 2;
  }
  if (kResidentRecomp.rec_func_index(program.entry) < 0 || kResidentRecomp.rec_func_index(program.boundary) < 0) {
    std::fprintf(stderr,
                 "REFUSED: %.*s generated substrate omits entry 0x%08X or boundary 0x%08X\n",
                 static_cast<int>(program.codeword.size()),
                 program.codeword.data(),
                 program.entry,
                 program.boundary);
    return 2;
  }

  psxport_install_game(program.runtime);
  psxport_install_recomp(&kResidentRecomp);
  auto game = std::make_unique<Game>();
  Core *core = &game->core;
  load_exe(program.executable, core);
  kResidentRecomp.shard_set_override(program.boundary, program.boundaryHandler);
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
