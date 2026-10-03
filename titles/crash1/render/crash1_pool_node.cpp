#include "crash1_pool_node.h"

namespace crash1::pool_node {
namespace {

constexpr std::uint32_t readWord(const Memory &memory, std::uint32_t address) noexcept {
  return memory.read(memory.context, address);
}

} // namespace

std::uint32_t bucketAddress(const Memory &memory, std::uint32_t classWord) noexcept {
  // 0x80012FC0..0x80012FD0, and byte-for-byte the same four instructions as 0x8001597C..0x80015988.
  // The shift selects a BUCKET; the class the cell carries is `classWord`, whole.
  const std::uint32_t table = readWord(memory, kBucketTable);
  return table + ((classWord >> kBucketIndexShift) & kBucketIndexMask);
}

std::uint32_t takeNode(const Memory &memory, std::uint32_t index) noexcept {
  // 0x80012F14..0x80012F24: index*2, +index = 3n, *4 = 12n, -n = 11n, *4 = 44n. Written out rather
  // than collapsed to `index * 44` because the manifest records 44 and the probe re-derives it from
  // these five instructions, and a reader has to be able to see which five.
  std::uint32_t scaled = index << 1;
  scaled += index;
  scaled <<= 2;
  scaled -= index;
  scaled <<= 2;
  const std::uint32_t node = kNodeTable + scaled;
  // 0x80012F54..0x80012F60: when the published cursor already names this node, the cursor is cleared.
  // Not an optimisation to be preserved "faithfully" — it is the only thing standing between two
  // callers and the same node, and a model without it would double-book.
  if (readWord(memory, kNodeCursor) == node) {
    memory.write(memory.context, kNodeCursor, 0u);
  }
  return node;
}

CellLink linkFirstCell(const Memory &memory, std::uint32_t node, std::uint32_t classWord) noexcept {
  CellLink link{0u, classWord, (classWord >> kBucketIndexShift) & kBucketIndexMask, 0u, false};
  std::uint32_t cell = readWord(memory, bucketAddress(memory, classWord));
  // 0x80012FE4 `addiu $17,$19,4`: the walk reads the class field, which is the SECOND word, so the
  // cursor starts one word into the cell rather than at it. The same half-stride `crash1_block_pool`
  // encodes as `kClassFieldOffset`.
  std::uint32_t probe = cell + 4u;
  while (readWord(memory, probe) != classWord) {
    probe += kCellStride; // 0x8001300C `addiu $17,$17,8`
    cell += kCellStride;  // 0x8001301C, the `bne` delay slot: it steps unconditionally
    ++link.cellsWalked;
    // 0x8001300C..0x80013018 is UNBOUNDED, exactly as the lookup at 0x800159A8 is. This model stops
    // at the edge of the same 2 MiB the lookup owner stops at, for the same reason: retail reads there
    // with no check at all, so a walk that ran off is a walk retail cannot complete, and returning the
    // offending address is more useful than answering it.
    if ((probe & 0xFFE00000u) != 0x80000000u) {
      link.exhausted = true;
      break;
    }
  }
  link.cell = cell;
  if (!link.exhausted) {
    // 0x80013020 `sw $5,0($19)`: the cell's FIRST word is the payload. The class word is NOT written
    // here — it was already in the cell, which is why this function had to find it by walking.
    memory.write(memory.context, cell, readWord(memory, node + kNodeElementArray));
  }
  return link;
}

} // namespace crash1::pool_node
