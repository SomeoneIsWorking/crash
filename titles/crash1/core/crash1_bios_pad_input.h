#pragma once

#include <cstdint>

struct Core;

namespace crash1::bios_pad_input {

// Publish the framework's standard active-low PSX mask through Crash 1's BIOS
// PadRead word. Port 1 remains disconnected until the product owns a second
// host controller.
std::uint32_t wordAddress();
void publishPrimary(Core &core, std::uint16_t activeLowButtons);

} // namespace crash1::bios_pad_input
