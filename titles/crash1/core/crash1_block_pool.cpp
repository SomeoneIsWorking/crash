#include "crash1_block_pool.h"

#include "core.h"
#include "crash1_runtime.h"
#include "dynarec_dispatch.h"
#include "game.h"

#include <cstdlib>
#include <lucent/log.h>

namespace crash1 {
namespace {

std::uint32_t readGuestCellClass(const void *context, std::uint32_t cell) {
  // `Core::mem_r32` is not const, and this reader is handed a `const void *` because the recovered
  // walk must not be able to change the guest through it. The const is dropped HERE, in one place,
  // and the drop is stated rather than smuggled: a read is a read, and the alternative would be
  // making the framework's accessor const, which this repository does not own.
  auto &core = *const_cast<Core *>(static_cast<const Core *>(context));
  return core.mem_r32(cell + kClassFieldOffset);
}

void findCellOverride(Core *core) {
  Crash1BlockPool &pool = Crash1BlockPool::from(*core);
  core->r[2] = pool.findCell(*core, core->r[4]);
}

} // namespace

CellSearch searchCells(const void *context,
                       const CellClassReader &read,
                       std::uint32_t firstCell,
                       std::uint32_t classKey) noexcept {
  CellSearch search{firstCell, 0u, false};
  // The FIRST cell is tested at 0x80015994 with no check at all, and that unchecked read is exactly
  // the fault this run took: the guest published 0x00800000, the read at 0x80015994 plus 4 is
  // `0x00800004`, and the framework's memory model does not map it. So the first cell needs the same
  // guard as every other one. It remains faithful — retail cannot execute an unmapped read — and the
  // owner returns the offending address unchanged, so the guest's own next instruction faults on the
  // same address retail would have faulted on rather than on a substituted value.
  if (!isMainRam(firstCell)) {
    search.leftMainRam = true;
    return search;
  }
  if (read(context, firstCell) == classKey) {
    return search;
  }
  for (std::uint32_t cell = firstCell + kCellStride;; cell += kCellStride) {
    ++search.cellsWalked;
    // The one rule this owner adds, and it is a host-memory fact rather than a pool-structure guess:
    // retail reads this cell with no check at all, so retail faults here, and a cell outside the
    // guest's own 2 MiB of main RAM is a cell no execution of retail can have reached. Stopping there
    // cannot change the answer for any case retail completes.
    if (!isMainRam(cell)) {
      search.foundCell = cell;
      search.leftMainRam = true;
      return search;
    }
    if (read(context, cell) == classKey) {
      search.foundCell = cell;
      return search;
    }
  }
}

std::uint32_t Crash1BlockPool::findCell(Core &core, std::uint32_t request) {
  if (firstCaller_ == 0u) {
    firstCaller_ = core.r[31];
  }

  // 0x80015978..0x80015988: the class key, and the bucket this class lives in. The index is a BYTE
  // offset into the table rather than an element index, which is why the mask is 0x3FC and not 0xFF.
  const std::uint32_t classKey = request >> kClassShift;
  const std::uint32_t bucketTable = core.mem_r32(kBucketTablePointer);
  ++lookups_;
  lastRequest_ = request;
  lastClassKey_ = classKey;
  lastBucketTable_ = bucketTable;
  lastPoolBase_ = core.mem_r32(kPoolBasePointer);
  lastLiveCellCount_ = core.mem_r32(core.mem_r32(kCellCountBlockPointer) + kCellCountOffset);

  // 0x8001598C: the bucket's first cell.
  const std::uint32_t firstCell = core.mem_r32(bucketTable + (classKey & kBucketIndexMask));
  lastFirstCell_ = firstCell;
  lastCellsWalked_ = 0u;

  const CellSearch search = searchCells(&core, readGuestCellClass, firstCell, classKey);
  lastCellsWalked_ = search.cellsWalked;
  if (search.cellsWalked > maxCellsWalked_) {
    maxCellsWalked_ = search.cellsWalked;
  }
  // The engine's own bound, EVALUATED and reported rather than enforced — see the header. Signed,
  // because the guest's pair is `sra` then `slt`, and negative for a bucket below the published base.
  lastPoolBaseDistance_ = static_cast<std::int32_t>(firstCell - lastPoolBase_) / static_cast<std::int32_t>(kCellStride);

  if (search.leftMainRam) {
    ++leftMainRam_;
    // Reported on the transition rather than on every call: this is a state change, and the
    // denominator is right here in the line so the count is never a bare number.
    lucent::error("crash1-pool",
                  "SCUS-949.00 block pool: the walk for class {} left main RAM at 0x{:08X} after {} "
                  "cell(s) from first cell 0x{:08X}; the guest asked 0x{:08X}, the bucket table is "
                  "0x{:08X}, the engine's own base 0x{:08X} is {} cell(s) away and it publishes {} "
                  "live cell(s) — across {} lookups served, {} found a cell, {} left main RAM, first "
                  "caller 0x{:08X}",
                  classKey,
                  search.foundCell,
                  search.cellsWalked,
                  firstCell,
                  request,
                  bucketTable,
                  lastPoolBase_,
                  lastPoolBaseDistance_,
                  lastLiveCellCount_,
                  lookups_,
                  found_,
                  leftMainRam_,
                  firstCaller_);
    return search.foundCell;
  }
  ++found_;
  return search.foundCell;
}

Crash1BlockPool &Crash1BlockPool::from(Core &core) {
  if (!core.runtime) {
    lucent::error("crash1-pool", "SCUS-949.00 block-pool override ran without its title runtime");
    std::abort();
  }
  // Same reasoning as `Crash1HorizontalBound::from`: the owner is per-title state behind the
  // runtime, so the downcast IS the lookup, and a Core running another title is a named refusal
  // rather than a neighbour's counters.
  auto *const runtime = dynamic_cast<Crash1Runtime *>(const_cast<GameRuntime *>(core.runtime));
  if (!runtime) {
    lucent::error("crash1-pool", "SCUS-949.00 block-pool override reached another title's runtime");
    std::abort();
  }
  return runtime->blockPool();
}

void installCrash1BlockPool(Core &core) {
  if (!crash::dynarec::installOverride(core, kFindCell, "Crash block-pool cell lookup", findCellOverride)) {
    std::abort();
  }
  lucent::info("crash1-pool",
               "block-pool owner installed: cell lookup 0x{:08X} (the stop site is 0x{:08X}); bucket "
               "table 0x{:08X}, pool base 0x{:08X}, live count at 0x{:08X}+0x{:X}; the walk is "
               "stopped at the edge of main RAM, NOT at the engine's own bound at 0x{:08X}, which was "
               "applied and measured to break a path retail completes; 6 measured call sites",
               kFindCell,
               kFaultSite,
               kBucketTablePointer,
               kPoolBasePointer,
               kCellCountBlockPointer,
               kCellCountOffset,
               kFindCellBounded);
}

} // namespace crash1
