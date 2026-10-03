#include "crash1_block_pool.h"

#include "core.h"
#include "crash1_runtime.h"
#include "game.h"
#include "native_dispatch.h"

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

CellSearch
searchCells(const void *context, const CellClassReader &read, std::uint32_t firstCell, std::uint32_t request) noexcept {
  CellSearch search{firstCell, 0u, false};
  // THE CLASS IS `request`, THE WHOLE WORD. `beq $2,$4` at 0x8001599C and `bne $2,$4` at 0x800159B0
  // both compare the cell's class field against register `$a0`, and nothing between the entry and
  // those branches redefines `$a0`. The `srl` at 0x80015978 exists only to index the bucket table and
  // its result is dead after 0x80015988. An owner that searched for the SHIFTED key here can never
  // match a cell the engine wrote, which is exactly what happened: every lookup ran off the end of
  // main RAM and handed the caller a cell at 0x80200000.
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
  if (read(context, firstCell) == request) {
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
    if (read(context, cell) == request) {
      search.foundCell = cell;
      return search;
    }
  }
}

std::uint32_t Crash1BlockPool::findCell(Core &core, std::uint32_t request) {
  if (firstCaller_ == 0u) {
    firstCaller_ = core.r[31];
  }

  // 0x80015978..0x80015988: the BUCKET this request's class lives in, and nothing else. The shift's
  // result is consumed by the `andi`/`addu` and is dead by 0x8001598C; the class the walk compares is
  // `request` itself, so `classKey` here names the bucket selector and NOT the class. The index is a
  // BYTE offset into the table rather than an element index, which is why the mask is 0x3FC and not
  // 0xFF.
  const std::uint32_t classKey = (request >> kClassShift) & kBucketIndexMask;
  const std::uint32_t bucketTable = core.mem_r32(kBucketTablePointer);
  ++lookups_;
  lastRequest_ = request;
  lastClassKey_ = classKey;
  lastBucketTable_ = bucketTable;
  lastPoolBase_ = core.mem_r32(kPoolBasePointer);
  lastLiveCellCount_ = core.mem_r32(core.mem_r32(kCellCountBlockPointer) + kCellCountOffset);

  // 0x8001598C: the bucket's first cell.
  const std::uint32_t firstCell = core.mem_r32(bucketTable + classKey);
  lastFirstCell_ = firstCell;
  lastCellsWalked_ = 0u;

  const CellSearch search = searchCells(&core, readGuestCellClass, firstCell, request);
  lastCellsWalked_ = search.cellsWalked;
  if (search.cellsWalked > maxCellsWalked_) {
    maxCellsWalked_ = search.cellsWalked;
  }
  // The engine's own bound, EVALUATED and reported rather than enforced — see the header. Signed,
  // because the guest's pair is `sra` then `slt`, and negative for a bucket below the published base.
  lastPoolBaseDistance_ = static_cast<std::int32_t>(firstCell - lastPoolBase_) / static_cast<std::int32_t>(kCellStride);

  if (search.leftMainRam) {
    ++leftMainRam_;
    lucent::error("crash1-pool",
                  "SCUS-949.00 block pool: the walk for the class of request 0x{:08X} (bucket byte "
                  "offset {}) left main RAM at 0x{:08X} after {} cell(s) from first cell 0x{:08X}; the "
                  "bucket table is 0x{:08X}, the engine's own base 0x{:08X} is {} cell(s) away and it "
                  "publishes {} live cell(s) — across {} lookups served, {} found a cell, {} left main "
                  "RAM, first caller 0x{:08X}",
                  request,
                  classKey,
                  search.foundCell,
                  search.cellsWalked,
                  firstCell,
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
  if (!reportedFirstLookup_) {
    reportedFirstLookup_ = true;
    // The first lookup of the run, whatever its outcome, so a log that goes on to print no pool error
    // at all can still be told apart from a run in which the owner was never reached.
    lucent::info("crash1-pool",
                 "SCUS-949.00 block pool: the first lookup of this run — request 0x{:08X} (bucket "
                 "byte offset {}), bucket table 0x{:08X}, first cell 0x{:08X}, engine base 0x{:08X} "
                 "publishing {} live cell(s); 1 of 1 lookups served so far, caller 0x{:08X}",
                 request,
                 classKey,
                 bucketTable,
                 firstCell,
                 lastPoolBase_,
                 lastLiveCellCount_,
                 firstCaller_);
  }
  ++found_;
  if (!reportedFirstFound_) {
    reportedFirstFound_ = true;
    // The FIRST cell the walk actually served, with the run's live denominators beside it. Without
    // this line a healthy run is silent, and "no pool error appeared" is indistinguishable from
    // "the owner never ran" — which is the same dead-instrument failure the `OtAttr` span count is.
    lucent::info("crash1-pool",
                 "SCUS-949.00 block pool: the first lookup that found a cell — request 0x{:08X} "
                 "(bucket byte offset {}), cell 0x{:08X} after {} cell(s) walked, first cell of the "
                 "bucket 0x{:08X}; {}-of-{} lookups served have found a cell, {}-of-{} left main RAM, "
                 "caller 0x{:08X}",
                 request,
                 classKey,
                 search.foundCell,
                 search.cellsWalked,
                 firstCell,
                 found_,
                 lookups_,
                 leftMainRam_,
                 lookups_,
                 firstCaller_);
  }
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
  psx::cpu::installNativeOverride(core, kFindCell, "Crash block-pool cell lookup", findCellOverride);
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
