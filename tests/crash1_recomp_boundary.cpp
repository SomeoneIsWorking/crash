#include "core.h"
#include "crash1_runtime.h"
#include "game.h"
#include "rec_decls.h"

#include <array>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string_view>
#include <system_error>

void load_exe(const char *path, Core *core);
int rec_func_index(std::uint32_t address);

namespace {

constexpr std::array<std::string_view, 32> kRegisterNames = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0",   "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
};

std::uint32_t parse_address(std::string_view text, const char *label) {
  if (text.starts_with("0x") || text.starts_with("0X")) {
    text.remove_prefix(2);
  }
  std::uint32_t value = 0;
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, 16);
  if (error != std::errc{} || end != text.data() + text.size()) {
    std::fprintf(stderr, "REFUSED: %s is not a hexadecimal guest address\n", label);
    std::exit(2);
  }
  return value;
}

void capture_call_boundary(Core *core) {
  std::printf("# PORT-CALL-BOUNDARY-REGS pc=0x%08X\n", core->pc);
  for (std::size_t index = 1; index < kRegisterNames.size(); ++index) {
    std::printf("# PORT-CALL-BOUNDARY-REG %.*s=0x%08X\n",
                static_cast<int>(kRegisterNames[index].size()),
                kRegisterNames[index].data(),
                core->r[index]);
  }
  std::printf("# PORT-CALL-BOUNDARY-REG lo=0x%08X\n", core->lo);
  std::printf("# PORT-CALL-BOUNDARY-REG hi=0x%08X\n", core->hi);
  std::fflush(stdout);
  std::exit(EXIT_SUCCESS);
}

void capture_enter_critical(Core *core) {
  constexpr std::uint32_t kLoadEnterCriticalSelector = 0x24040001;
  constexpr std::uint32_t kSyscallZero = 0x0000000C;
  if (core->mem_r32(core->pc) != kLoadEnterCriticalSelector || core->mem_r32(core->pc + 4) != kSyscallZero) {
    std::fprintf(stderr,
                 "REFUSED: requested syscall boundary 0x%08X is not Crash 1's measured "
                 "addiu-a0-1/syscall-0 wrapper\n",
                 core->pc);
    std::exit(2);
  }

  const int irq_before = core->game->hle.irq_enabled;
  const std::uint32_t status_before = core->cop0[12];
  gen_func_8003E1F8(core);
  std::printf("# PORT-ENTER-CRITICAL boundary=0x%08X selector=0x%08X\n", core->pc, core->r[4]);
  std::printf("# PORT-ENTER-CRITICAL v0=0x%08X\n", core->r[2]);
  std::printf("# PORT-ENTER-CRITICAL irq-before=%d irq-after=%d\n", irq_before, core->game->hle.irq_enabled);
  std::printf("# PORT-ENTER-CRITICAL status-before=0x%08X status-after=0x%08X\n", status_before, core->cop0[12]);
  std::fflush(stdout);
  std::exit(EXIT_SUCCESS);
}

} // namespace

int main(int argc, char **argv) {
  if (argc != 4 && argc != 5) {
    std::fprintf(stderr, "usage: %s <PS-X EXE> <entry> <call-target> [--execute-enter-critical]\n", argv[0]);
    return 2;
  }

  const std::uint32_t entry = parse_address(argv[2], "entry");
  const std::uint32_t boundary = parse_address(argv[3], "call-target");
  const bool execute_enter_critical = argc == 5 && std::string_view(argv[4]) == "--execute-enter-critical";
  if (argc == 5 && !execute_enter_critical) {
    std::fprintf(stderr, "REFUSED: unknown boundary mode %s\n", argv[4]);
    return 2;
  }
  if (rec_func_index(entry) < 0 || rec_func_index(boundary) < 0) {
    std::fprintf(stderr, "REFUSED: generated substrate omits entry 0x%08X or boundary 0x%08X\n", entry, boundary);
    return 2;
  }

  // Process-lifetime game owner. The boundary runner deliberately drives generated execution
  // directly below; Crash1Runtime refuses the later native-boot path until the BIOS transition has
  // been modeled instead of bypassing that frontier.
  static crash1::Crash1Runtime runtime;
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  Core *core = &game->core;
  load_exe(argv[1], core);
  shard_set_override(boundary, execute_enter_critical ? capture_enter_critical : capture_call_boundary);
  main_dispatch(core, entry);
  std::fprintf(stderr, "FAIL: entry 0x%08X returned without reaching boundary 0x%08X\n", entry, boundary);
  return 1;
}
