#pragma once

#include "core.h"
#include "enter_critical_frontier.h"
#include "rec_decls.h"
#include "resident_program.h"

#include <array>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <system_error>

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

inline crash::EnterCriticalResult
requireEnterCritical(Core *core, std::string_view codeword, OverrideFn generatedBody) {
  const crash::EnterCriticalResult result = crash::runEnterCriticalFrontier(*core, codeword, generatedBody);
  if (!result.accepted) {
    std::fprintf(stderr, "REFUSED: %s\n", result.detail.c_str());
    std::exit(2);
  }
  return result;
}

inline void printEnterCritical(std::string_view codeword, const crash::EnterCriticalObservation &observation) {
  std::printf("# %.*s-PORT-ENTER-CRITICAL boundary=0x%08X selector=0x%08X\n",
              static_cast<int>(codeword.size()),
              codeword.data(),
              observation.boundary,
              observation.selector);
  std::printf("# %.*s-PORT-ENTER-CRITICAL v0=0x%08X\n",
              static_cast<int>(codeword.size()),
              codeword.data(),
              observation.returnValue);
  std::printf("# %.*s-PORT-ENTER-CRITICAL irq-before=%d irq-after=%d\n",
              static_cast<int>(codeword.size()),
              codeword.data(),
              observation.irqBefore,
              observation.irqAfter);
  std::printf("# %.*s-PORT-ENTER-CRITICAL status-before=0x%08X status-after=0x%08X\n",
              static_cast<int>(codeword.size()),
              codeword.data(),
              observation.statusBefore,
              observation.statusAfter);
  std::printf("# %.*s-PORT-ENTER-CRITICAL cause-before=0x%08X cause-after=0x%08X\n",
              static_cast<int>(codeword.size()),
              codeword.data(),
              observation.causeBefore,
              observation.causeAfter);
  std::printf("# %.*s-PORT-ENTER-CRITICAL epc-before=0x%08X epc-after=0x%08X\n",
              static_cast<int>(codeword.size()),
              codeword.data(),
              observation.epcBefore,
              observation.epcAfter);
  std::fflush(stdout);
}

inline void executeEnterCritical(Core *core, std::string_view codeword, OverrideFn generatedBody) {
  const crash::EnterCriticalResult result = requireEnterCritical(core, codeword, generatedBody);
  printEnterCritical(codeword, result.observation);
  std::exit(EXIT_SUCCESS);
}

inline void resumeEnterCritical(Core *core, std::string_view codeword, OverrideFn generatedBody) {
  const crash::EnterCriticalResult result = requireEnterCritical(core, codeword, generatedBody);
  printEnterCritical(codeword, result.observation);
}

template <typename Runtime>
int runBoundary(int argc,
                char **argv,
                std::string_view codeword,
                OverrideFn captureCall,
                OverrideFn enterCritical,
                OverrideFn resumeCritical) {
  if (argc != 4 && argc != 5 && argc != 6) {
    std::fprintf(stderr,
                 "usage: %s <PS-X EXE> <entry> <call-target> "
                 "[--execute-enter-critical | --resume-enter-critical-to <call-target>]\n",
                 argv[0]);
    return 2;
  }

  const std::uint32_t entry = parseAddress(argv[2], "entry");
  const std::uint32_t boundary = parseAddress(argv[3], "call-target");
  const bool executeEnter = argc == 5 && std::string_view(argv[4]) == "--execute-enter-critical";
  const bool resumeEnter = argc == 6 && std::string_view(argv[4]) == "--resume-enter-critical-to";
  if ((argc == 5 && !executeEnter) || (argc == 6 && !resumeEnter)) {
    std::fprintf(stderr, "REFUSED: unknown boundary mode %s\n", argv[4]);
    return 2;
  }
  const std::uint32_t finalBoundary = resumeEnter ? parseAddress(argv[5], "post-syscall call-target") : boundary;
  static Runtime runtime;
  return crash::runResidentProgram({
      .runtime = runtime,
      .codeword = codeword,
      .executable = argv[1],
      .entry = entry,
      .boundary = finalBoundary,
      .boundaryHandler = executeEnter ? enterCritical : captureCall,
      .boundaryKind =
          resumeEnter ? crash::ResidentBoundaryKind::DynamicDispatch : crash::ResidentBoundaryKind::GeneratedEntry,
      .transitionBoundary = resumeEnter ? boundary : 0,
      .transitionHandler = resumeEnter ? resumeCritical : nullptr,
  });
}

template <typename Runtime> class TitleBoundaryRunner {
public:
  static int run(int argc, char **argv, std::string_view codeword, OverrideFn generatedEnterCritical) {
    codeword_ = codeword;
    generatedEnterCritical_ = generatedEnterCritical;
    return runBoundary<Runtime>(argc, argv, codeword, captureCall, captureEnterCritical, resumeFromEnterCritical);
  }

private:
  static void captureCall(Core *core) {
    captureCallBoundary(core, codeword_);
  }

  static void captureEnterCritical(Core *core) {
    executeEnterCritical(core, codeword_, generatedEnterCritical_);
  }

  static void resumeFromEnterCritical(Core *core) {
    resumeEnterCritical(core, codeword_, generatedEnterCritical_);
  }

  static inline std::string_view codeword_;
  static inline OverrideFn generatedEnterCritical_ = nullptr;
};

} // namespace crash::test
