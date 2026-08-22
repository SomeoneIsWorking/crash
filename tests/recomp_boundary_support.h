#pragma once

#include "core.h"
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

namespace crash::test {

inline constexpr std::array<std::string_view, 32> kRegisterNames = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0",   "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
};

inline std::uint32_t parseAddress(std::string_view text, const char *label) {
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

inline void captureCallBoundary(Core *core, std::string_view codeword) {
  std::printf(
      "# %.*s-PORT-CALL-BOUNDARY-REGS pc=0x%08X\n", static_cast<int>(codeword.size()), codeword.data(), core->pc);
  for (std::size_t index = 1; index < kRegisterNames.size(); ++index) {
    std::printf("# %.*s-PORT-CALL-BOUNDARY-REG %.*s=0x%08X\n",
                static_cast<int>(codeword.size()),
                codeword.data(),
                static_cast<int>(kRegisterNames[index].size()),
                kRegisterNames[index].data(),
                core->r[index]);
  }
  std::printf(
      "# %.*s-PORT-CALL-BOUNDARY-REG lo=0x%08X\n", static_cast<int>(codeword.size()), codeword.data(), core->lo);
  std::printf(
      "# %.*s-PORT-CALL-BOUNDARY-REG hi=0x%08X\n", static_cast<int>(codeword.size()), codeword.data(), core->hi);
  std::fflush(stdout);
  std::exit(EXIT_SUCCESS);
}

inline void executeEnterCritical(Core *core, std::string_view codeword, OverrideFn generatedBody) {
  constexpr std::uint32_t kLoadEnterCriticalSelector = 0x24040001;
  constexpr std::uint32_t kSyscallZero = 0x0000000C;
  if (core->mem_r32(core->pc) != kLoadEnterCriticalSelector || core->mem_r32(core->pc + 4) != kSyscallZero) {
    std::fprintf(stderr,
                 "REFUSED: requested syscall boundary 0x%08X is not %.*s's measured "
                 "addiu-a0-1/syscall-0 wrapper\n",
                 core->pc,
                 static_cast<int>(codeword.size()),
                 codeword.data());
    std::exit(2);
  }

  const int irqBefore = core->game->hle.irq_enabled;
  const std::uint32_t statusBefore = core->cop0[12];
  generatedBody(core);
  std::printf("# %.*s-PORT-ENTER-CRITICAL boundary=0x%08X selector=0x%08X\n",
              static_cast<int>(codeword.size()),
              codeword.data(),
              core->pc,
              core->r[4]);
  std::printf("# %.*s-PORT-ENTER-CRITICAL v0=0x%08X\n", static_cast<int>(codeword.size()), codeword.data(), core->r[2]);
  std::printf("# %.*s-PORT-ENTER-CRITICAL irq-before=%d irq-after=%d\n",
              static_cast<int>(codeword.size()),
              codeword.data(),
              irqBefore,
              core->game->hle.irq_enabled);
  std::printf("# %.*s-PORT-ENTER-CRITICAL status-before=0x%08X status-after=0x%08X\n",
              static_cast<int>(codeword.size()),
              codeword.data(),
              statusBefore,
              core->cop0[12]);
  std::fflush(stdout);
  std::exit(EXIT_SUCCESS);
}

template <typename Runtime>
int runBoundary(int argc, char **argv, std::string_view codeword, OverrideFn captureCall, OverrideFn enterCritical) {
  if (argc != 4 && argc != 5) {
    std::fprintf(stderr, "usage: %s <PS-X EXE> <entry> <call-target> [--execute-enter-critical]\n", argv[0]);
    return 2;
  }

  const std::uint32_t entry = parseAddress(argv[2], "entry");
  const std::uint32_t boundary = parseAddress(argv[3], "call-target");
  const bool executeEnter = argc == 5 && std::string_view(argv[4]) == "--execute-enter-critical";
  if (argc == 5 && !executeEnter) {
    std::fprintf(stderr, "REFUSED: unknown boundary mode %s\n", argv[4]);
    return 2;
  }
  if (rec_func_index(entry) < 0 || rec_func_index(boundary) < 0) {
    std::fprintf(stderr,
                 "REFUSED: %.*s generated substrate omits entry 0x%08X or boundary 0x%08X\n",
                 static_cast<int>(codeword.size()),
                 codeword.data(),
                 entry,
                 boundary);
    return 2;
  }

  static Runtime runtime;
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  Core *core = &game->core;
  load_exe(argv[1], core);
  shard_set_override(boundary, executeEnter ? enterCritical : captureCall);
  main_dispatch(core, entry);
  std::fprintf(stderr,
               "FAIL: %.*s entry 0x%08X returned without reaching boundary 0x%08X\n",
               static_cast<int>(codeword.size()),
               codeword.data(),
               entry,
               boundary);
  return 1;
}

} // namespace crash::test
