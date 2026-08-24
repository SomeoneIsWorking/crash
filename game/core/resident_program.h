#pragma once

#include "recomp_iface.h"

#include <cstdint>
#include <string_view>

class GameRuntime;

namespace crash {

struct ResidentProgram {
  GameRuntime &runtime;
  std::string_view codeword;
  const char *executable;
  std::uint32_t entry;
  std::uint32_t boundary;
  RecOverrideFn boundaryHandler;
};

// Install the title runtime and this target's generated resident substrate, load the verified
// executable, and dispatch from its real crt0 entry. The boundary handler owns the current honest
// stop; returning from the generated entry without reaching it is a failure.
int runResidentProgram(const ResidentProgram &program);

} // namespace crash
