#pragma once

#include "psx_exe_image.h"

#include <cstddef>
#include <filesystem>
#include <string_view>

class Core;

namespace crash1 {

// Authenticates the bytes that will be mapped, rather than a prior read of the same path.
psx::cpu::PsxExeLoadResult loadVerifiedExecutable(Core &core,
                                                  const std::filesystem::path &path,
                                                  std::size_t expectedSize,
                                                  std::string_view expectedSha256);

// The retail identity is taken from titles/crash1/executable.json at configure time.
psx::cpu::PsxExeLoadResult loadResidentExecutable(Core &core, const std::filesystem::path &path);

} // namespace crash1
