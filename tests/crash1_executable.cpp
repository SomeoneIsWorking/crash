#include "crash1_executable.h"

#include "core.h"

#include <lucent/content.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kEntry = 0x80010000u;
constexpr std::uint32_t kWord = 0x12345678u;
constexpr std::array<char, 8> kMagic{'P', 'S', '-', 'X', ' ', 'E', 'X', 'E'};

void writeWord(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value) {
  for (unsigned shift = 0; shift < 32; shift += 8) {
    bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
  }
}

std::vector<std::uint8_t> fixture(std::uint32_t stackBase = 0x801ffff0u) {
  std::vector<std::uint8_t> bytes(psx::cpu::kPsxExeHeaderBytes + 4u);
  std::copy(kMagic.begin(), kMagic.end(), bytes.begin());
  writeWord(bytes, 0x10, kEntry);
  writeWord(bytes, 0x14, 0x80050100u);
  writeWord(bytes, 0x18, kEntry);
  writeWord(bytes, 0x1c, 4u);
  writeWord(bytes, 0x30, stackBase);
  writeWord(bytes, psx::cpu::kPsxExeHeaderBytes, kWord);
  return bytes;
}

bool writeFile(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  file.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  file.close();
  return static_cast<bool>(file);
}

bool unchanged(Core &core) {
  return core.pc == 0x80010100u && core.r[28] == 0x11111111u && core.r[29] == 0x22222222u &&
         core.r[30] == 0x44444444u && core.r[31] == 0x33333333u && core.mem_r32(kEntry) == 0u &&
         !core.currentImageIdentity(kEntry);
}

} // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    std::fprintf(stderr, "Crash executable test needs one scratch directory\n");
    return 2;
  }
  std::filesystem::path directory = argv[1];
  std::error_code error;
  std::filesystem::create_directories(directory, error);
  if (error) {
    std::fprintf(stderr, "cannot create Crash executable test scratch directory: %s\n", error.message().c_str());
    return 2;
  }
  auto path = directory / "fixture.exe";
  auto bytes = fixture();
  if (!writeFile(path, bytes)) {
    std::fprintf(stderr, "cannot write Crash executable fixture\n");
    return 2;
  }
  auto digest =
      lucent::content::sha256_hex(lucent::content::sha256(std::as_bytes(std::span<const std::uint8_t>(bytes))));
  auto core = std::make_unique<Core>();
  core->pc = 0x80010100u;
  core->r[28] = 0x11111111u;
  core->r[29] = 0x22222222u;
  core->r[30] = 0x44444444u;
  core->r[31] = 0x33333333u;

  bool ok = true;
  auto retailRefusal = crash1::loadResidentExecutable(*core, path);
  ok &= !retailRefusal && unchanged(*core);
  auto wrongDigest = crash1::loadVerifiedExecutable(*core, path, bytes.size(), std::string(64, '0'));
  ok &= !wrongDigest && unchanged(*core);
  bytes.back() ^= 1u;
  ok &= writeFile(path, bytes);
  auto alteredBytes = crash1::loadVerifiedExecutable(*core, path, bytes.size(), digest);
  ok &= !alteredBytes && unchanged(*core);
  bytes.pop_back();
  ok &= writeFile(path, bytes);
  auto shortFile = crash1::loadVerifiedExecutable(*core, path, bytes.size() + 1u, digest);
  ok &= !shortFile && unchanged(*core);

  bytes = fixture();
  ok &= writeFile(path, bytes);
  auto loaded = crash1::loadVerifiedExecutable(*core, path, bytes.size(), digest);
  ok &= static_cast<bool>(loaded) && core->pc == kEntry && core->r[28] == 0x80050100u && core->r[29] == 0x801ffff0u &&
        core->r[30] == 0x801ffff0u && core->r[31] == 0xdead0000u && core->mem_r32(kEntry) == kWord &&
        core->currentImageIdentity(kEntry).has_value();

  bytes = fixture(0);
  ok &= writeFile(path, bytes);
  auto noStackDigest =
      lucent::content::sha256_hex(lucent::content::sha256(std::as_bytes(std::span<const std::uint8_t>(bytes))));
  auto noStackCore = std::make_unique<Core>();
  noStackCore->r[29] = 0x22222222u;
  noStackCore->r[30] = 0x44444444u;
  noStackCore->r[31] = 0x33333333u;
  auto noStackLoaded = crash1::loadVerifiedExecutable(*noStackCore, path, bytes.size(), noStackDigest);
  ok &= static_cast<bool>(noStackLoaded) && noStackCore->pc == kEntry && noStackCore->r[29] == 0x801ffff0u &&
        noStackCore->r[30] == 0x801ffff0u && noStackCore->r[31] == 0xdead0000u;
  std::filesystem::remove(path, error);
  if (error) {
    std::fprintf(stderr, "cannot remove Crash executable fixture: %s\n", error.message().c_str());
    return 2;
  }
  if (!ok) {
    std::fprintf(stderr, "Crash executable authentication or unchanged-state contract failed\n");
    return 1;
  }
  std::printf("Crash 1 executable: altered digest and bytes refused; exact bytes mapped once\n");
  return 0;
}
