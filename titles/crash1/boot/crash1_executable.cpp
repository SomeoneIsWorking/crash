#include "crash1_executable.h"

#include "core.h"

#include <lucent/content.h>

#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#ifndef CRASH1_EXE_EXPECTED_SIZE
#error "Crash 1 executable size must come from titles/crash1/executable.json"
#endif
#ifndef CRASH1_EXE_SHA256
#error "Crash 1 executable SHA-256 must come from titles/crash1/executable.json"
#endif

namespace crash1 {
namespace {

constexpr std::string_view kImageName = "crash1-usa-resident";

psx::cpu::PsxExeLoadResult refuse(std::string detail) {
  return {std::nullopt, {}, std::move(detail)};
}

} // namespace

psx::cpu::PsxExeLoadResult loadVerifiedExecutable(Core &core,
                                                  const std::filesystem::path &path,
                                                  std::size_t expectedSize,
                                                  std::string_view expectedSha256) {
  if (expectedSize < psx::cpu::kPsxExeHeaderBytes || expectedSize > psx::cpu::kPsxExeMaxBytes) {
    return refuse("Crash 1 expected executable size is outside PS-X EXE bounds");
  }
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    return refuse("cannot open Crash 1 executable");
  }
  if (file.tellg() != static_cast<std::streamoff>(expectedSize)) {
    return refuse("Crash 1 executable size disagrees with its manifest");
  }
  file.seekg(0);
  std::vector<std::uint8_t> bytes(expectedSize);
  file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (file.gcount() != static_cast<std::streamsize>(bytes.size()) || file.peek() != std::ifstream::traits_type::eof()) {
    return refuse("Crash 1 executable changed size or could not be read completely");
  }
  auto digest = lucent::content::sha256_hex(
      lucent::content::sha256(std::as_bytes(std::span<const std::uint8_t>(bytes.data(), bytes.size()))));
  if (digest != expectedSha256) {
    return refuse("Crash 1 executable SHA-256 disagrees with its manifest");
  }
  auto loaded = psx::cpu::loadPsxExeImage(core, bytes, kImageName);
  if (!loaded) {
    return loaded;
  }
  psx::cpu::applyPsxExeTopLevelRegisters(core, loaded.image);
  return loaded;
}

psx::cpu::PsxExeLoadResult loadResidentExecutable(Core &core, const std::filesystem::path &path) {
  return loadVerifiedExecutable(core, path, CRASH1_EXE_EXPECTED_SIZE, CRASH1_EXE_SHA256);
}

} // namespace crash1
