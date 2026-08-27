#pragma once

#include "recomp_iface.h"

#include <cstdint>
#include <string_view>

class GameRuntime;

namespace crash {

enum class ResidentBoundaryKind {
  GeneratedEntry,
  DynamicDispatch,
};

struct ResidentProgram {
  GameRuntime &runtime;
  std::string_view codeword;
  const char *executable;
  std::uint32_t entry;
  std::uint32_t boundary;
  RecOverrideFn boundaryHandler;
  ResidentBoundaryKind boundaryKind{ResidentBoundaryKind::GeneratedEntry};
  std::uint32_t transitionBoundary{};
  RecOverrideFn transitionHandler{};
};

// Install one title runtime together with this target's generated resident substrate. Direct
// products use this before constructing Game; finite boundary tools use it through
// runResidentProgram. Keeping the registry construction here prevents the shipping loop and the
// differential frontier from acquiring separate generated-code seams.
void installResidentRuntime(GameRuntime &runtime);

// Install the title runtime and this target's generated resident substrate, load the verified
// executable, and dispatch from its real crt0 entry. A GeneratedEntry boundary intercepts a known
// shard body; DynamicDispatch observes the call before an override, shard, or BIOS/HLE leaf owns it.
// The optional transition is always a generated body and may return to continue toward the final
// boundary. The final handler owns the current honest stop; returning from the entry without
// reaching it is a failure.
int runResidentProgram(const ResidentProgram &program);

} // namespace crash
