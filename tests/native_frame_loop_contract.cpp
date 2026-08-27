#include "native_frame_loop_contract.h"
#include "crash1_runtime.h"
#include "crash2_runtime.h"
#include "crash3_runtime.h"
#include "game.h"
#include "game_runtime.h"
#include "platform_hle.h"

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

int gFailures = 0;

void forbiddenVSyncReplacement(Core *) {}

void check(bool condition, const char *detail) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", detail);
    ++gFailures;
  }
}

template <typename Runtime>
void checkStaticContract(const char *name,
                         std::uint32_t expectedVSync,
                         std::uint32_t expectedVSyncEnd,
                         crash::NativeFrameLoopState expectedState) {
  Runtime runtime;
  const crash::NativeFrameLoopContract &frame = runtime.nativeFrameLoopContract();
  const PlatformHlePlan *platform = runtime.platformHlePlan();
  check(frame.guestVSync.begin == expectedVSync, name);
  check(frame.guestVSync.end == expectedVSyncEnd, name);
  check(frame.state == expectedState, name);
  check(frame.canStepFrame() == (expectedState == crash::NativeFrameLoopState::Ready), name);
  check(platform != nullptr, name);
  if (platform != nullptr) {
    check(platform->vsyncAddress == expectedVSync, name);
    check(platform->windowLo[0] == expectedVSync, name);
    check(platform->windowHi[0] == expectedVSyncEnd, name);
    check(platform->bindingCount == 0, name);
  }
}

template <typename Runtime, typename Operation> void expectAbort(const char *name, Operation operation) {
  const pid_t pid = fork();
  if (pid < 0) {
    std::perror("fork");
    ++gFailures;
    return;
  }
  if (pid == 0) {
    const rlimit noCore{0, 0};
    setrlimit(RLIMIT_CORE, &noCore);
    Runtime runtime;
    psxport_install_game(runtime);
    auto game = std::make_unique<Game>();
    operation(runtime, *game);
    _exit(EXIT_SUCCESS);
  }

  int status = 0;
  if (waitpid(pid, &status, 0) != pid) {
    std::perror("waitpid");
    ++gFailures;
    return;
  }
  const bool aborted = WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT;
  check(aborted, name);
  std::printf("%s: %s\n", aborted ? "PASS" : "FAIL", name);
}

template <typename Runtime> void checkVSyncDeaths(const char *name) {
  const std::string prefix{name};
  expectAbort<Runtime>((prefix + " VSync(-1) refusal").c_str(), [](Runtime &runtime, Game &game) {
    crash::initializeNativeFrameLoopContract(game);
    OverrideFn vsync = game.platform_hle.lookup(runtime.nativeFrameLoopContract().guestVSync.begin);
    if (vsync == nullptr) {
      _exit(3);
    }
    game.core.r[4] = 0xFFFFFFFFu;
    vsync(&game.core);
  });
  expectAbort<Runtime>((prefix + " VSync(0) refusal").c_str(), [](Runtime &runtime, Game &game) {
    crash::initializeNativeFrameLoopContract(game);
    OverrideFn vsync = game.platform_hle.lookup(runtime.nativeFrameLoopContract().guestVSync.begin);
    if (vsync == nullptr) {
      _exit(3);
    }
    game.core.r[4] = 0;
    vsync(&game.core);
  });
  expectAbort<Runtime>((prefix + " VSync replacement refusal").c_str(), [](Runtime &runtime, Game &game) {
    crash::initializeNativeFrameLoopContract(game);
    game.platform_hle.register_(runtime.nativeFrameLoopContract().guestVSync.begin, forbiddenVSyncReplacement);
  });
}

template <typename Runtime> void checkRefusingFrameDeath(const char *name) {
  expectAbort<Runtime>(name, [](Runtime &, Game &game) {
    game.frameDriver->stepFrame(game.core, 0);
  });
}

} // namespace

int main() {
  checkStaticContract<crash1::Crash1Runtime>(
      "Crash 1 static contract", 0x8003E4F0u, 0x8003E638u, crash::NativeFrameLoopState::FiniteBootSeamOnly);
  checkStaticContract<crash2::Crash2Runtime>(
      "Crash 2 static contract", 0x8004A484u, 0x8004A5CCu, crash::NativeFrameLoopState::Missing);
  checkStaticContract<crash3::Crash3Runtime>(
      "Crash 3 static contract", 0x8004B2A8u, 0x8004B3F0u, crash::NativeFrameLoopState::Missing);

  checkVSyncDeaths<crash1::Crash1Runtime>("Crash 1 guest VSync aborts");
  checkRefusingFrameDeath<crash2::Crash2Runtime>("Crash 2 unproven frame aborts");
  checkVSyncDeaths<crash2::Crash2Runtime>("Crash 2 guest VSync aborts");
  checkRefusingFrameDeath<crash3::Crash3Runtime>("Crash 3 unproven frame aborts");
  checkVSyncDeaths<crash3::Crash3Runtime>("Crash 3 guest VSync aborts");

  std::printf("native frame contract: %s\n", gFailures == 0 ? "PASS" : "FAIL");
  return gFailures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
