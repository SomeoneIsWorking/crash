// Crash 1's process boot: compose the framework services this title ships, admit its authenticated
// executable, install its native owners, and hand the frame turn to psxport's spine.
#pragma once

#include <filesystem>
#include <memory>

class Game;

namespace crash1 {

class Crash1Runtime;

namespace boot {

// The disc this title reads. One environment key, read by the framework's own disc owner; the
// title never opens a disc itself.
inline constexpr const char *kDiscEnvironmentKey = "PSXPORT_CRASH1_DISC";

// The authenticated retail executable, inside the repository's gitignored scratch disc cache.
[[nodiscard]] const std::filesystem::path &defaultExecutablePath();

// Everything between `main` and the guest's first instruction, in one owner: the Game and the
// framework services it carries, the title's executable admission, the native overrides, and the
// run-end counters. `run` returns the process exit code.
class ProductBoot final {
public:
  explicit ProductBoot(Crash1Runtime &runtime);
  ~ProductBoot();

  [[nodiscard]] Game &game() const {
    return *game_;
  }

  [[nodiscard]] int run(const std::filesystem::path &executable);

private:
  void bindFrameworkDevices();
  void logExecutionCounters() const;

  Crash1Runtime &runtime_;
  std::unique_ptr<Game> game_;
};

} // namespace boot

} // namespace crash1