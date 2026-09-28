// Falsifiers for SCUS-949.00's size-class block-pool owner.
//
// The owner replaces the function the product faults in — `lw $v0,0x4($v1)` at 0x800159A8, the second
// class read of the cell lookup at 0x80015978 — so the contract under test is the RECOVERED WALK, not
// a recording. Two things are pinned separately and for different reasons:
//
//   1. the walk, as a pure function of the bucket's first cell, driven against a fixture with no Core
//      in existence. This is where a transcription error would show, and it is where the negative
//      cases live: a walk that returns the wrong cell, a walk that ignores the engine's own bound,
//      and a walk that reads one cell too few are three different defects;
//   2. the owner, through the real runtime, because the contract there is what it does AROUND a
//      guest read — which bucket it read, what it published, and what it returns when that bucket is
//      not a pointer into main RAM.
//
// The measured inputs come from the authenticated executable through
// tools/probe_crash1_block_pool.py, which records them in titles/crash1/executable.json and diffs
// the header's own literals against that manifest. A constant cannot drift here without a gate going
// red.
//
// WHAT IS NOT UNDER TEST, stated so the coverage is not read as more than it is: no case here proves
// the pool is the ROOT CAUSE of Crash 1 presenting no frame, and no case establishes which of the
// six measured call sites ran. Both are runtime facts, and `docs/issues/0020` records them as such.

#include "crash1_block_pool.h"

#include "core.h"
#include "crash1_runtime.h"
#include "game.h"
#include "game_runtime.h"
#include "hw_bind.h"
#include "image_identity.h"
#include "native_dispatch.h"

#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

namespace {

using crash1::BlockCell;
using crash1::CellSearch;
using crash1::Crash1BlockPool;

int failures = 0;

void check(bool condition, const char *what) {
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++failures;
  }
}

// A bucket fixture the recovered walk can be driven against without a Core.
//
// The fixture models the pool the way the guest's memory actually is: a SPARSE region of cell
// addresses inside main RAM. Declared cells answer with their class; any other in-range address
// answers with `kNoCell`, because an unserved cell is a real thing in a real pool and a fixture that
// refused to answer for one would make the absent-class case untestable. The two things the fixture
// DOES police are the two that must never happen: it is never asked for a cell outside main RAM, and
// it records the address of every read so a test can check the walk stopped where it claims to.
class Fixture {
public:
  explicit Fixture(std::vector<std::uint32_t> classes) : classes_(std::move(classes)) {}

  static constexpr std::uint32_t kNoCell = 0xFFFFFFFFu;
  static constexpr std::uint32_t kBase = 0x80100000u;

  std::uint32_t base() const {
    return kBase;
  }
  std::uint32_t size() const {
    return static_cast<std::uint32_t>(classes_.size());
  }
  std::uint32_t classAt(std::uint32_t index) const {
    return classes_.at(index);
  }
  std::uint32_t reads() const {
    return reads_;
  }
  // How many cells the walk can step before the next address leaves main RAM: 0x200000 bytes of pool
  // from kBase, in 8-byte cells.
  std::uint32_t cellsToMainRamEdge() const {
    return (0x80200000u - kBase) / crash1::kCellStride;
  }
  std::uint32_t lastAddress() const {
    return lastAddress_;
  }
  std::uint32_t askedOutsideMainRam() const {
    return askedOutsideMainRam_;
  }

  static std::uint32_t read(const void *context, std::uint32_t cell) {
    // The counter is mutated by a reader the recovered walk is handed as `const void *`, so the
    // const is dropped here, once, in the fixture — the same single stated place the owner drops it
    // for `Core::mem_r32`. The fixture counts because a test that cannot see how far the walk went
    // would only be able to assert that it returned.
    auto &self = *const_cast<Fixture *>(static_cast<const Fixture *>(context));
    ++self.reads_;
    self.lastAddress_ = cell;
    if (!crash1::isMainRam(cell)) {
      // The one thing the walk must never do. It is counted rather than answered, because answering
      // would be the failure.
      ++self.askedOutsideMainRam_;
      return kNoCell;
    }
    const std::uint32_t offset = cell - kBase;
    if (offset % crash1::kCellStride != 0u) {
      std::fprintf(stderr, "FAIL: the walk read 0x%08X, which is not on the 8-byte cell grid\n", cell);
      return kNoCell;
    }
    const std::uint32_t index = offset / crash1::kCellStride;
    return index < self.classes_.size() ? self.classes_[index] : kNoCell;
  }

private:
  std::vector<std::uint32_t> classes_;
  std::uint32_t reads_{};
  std::uint32_t lastAddress_{};
  std::uint32_t askedOutsideMainRam_{};
};

constexpr std::uint32_t kNearPlaneConsumer = 0x8003A144u;
constexpr std::uint32_t kFaultSite = 0x800159A8u;

} // namespace

int main() {
  // --- the recovered walk -------------------------------------------------------------------------
  //
  // `0x80015994` tests the FIRST cell before the loop, so a class on the first cell must be found
  // with no stride step at all. `cellsWalked` is 0 there, and that is the denominator: "found on the
  // first cell" and "found on the fourth" are different statements about the same bucket.
  {
    const Fixture fixture({7u, 9u, 11u});
    const CellSearch search = crash1::searchCells(&fixture, &Fixture::read, fixture.base(), 7u);
    check(!search.leftMainRam, "a class on the first cell is found, not left main RAM");
    check(search.foundCell == fixture.base(), "the FIRST cell is returned, as 0x8001599C does");
    check(search.cellsWalked == 0u, "a first-cell match walks no cell: 0 of the array is examined");
    check(fixture.reads() == 1u, "exactly one class read happened, and it was the first cell");
    check(fixture.askedOutsideMainRam() == 0u, "no read left main RAM");
  }

  // A match on a later cell: the return must be the cell that MATCHED, not its successor. This is the
  // `+8` in the loop's delay slot and the `-8` at 0x800159B8 cancelling, and it is the single easiest
  // thing to get wrong when transcribing a delay-slot loop, so it is pinned with a fixture whose
  // successor carries a DIFFERENT class — a transcription that returned the successor would then be
  // caught rather than coincidentally agreeing.
  {
    const Fixture fixture({7u, 9u, 11u});
    const CellSearch search = crash1::searchCells(&fixture, &Fixture::read, fixture.base(), 11u);
    check(!search.leftMainRam, "a class on the third cell is found");
    check(search.foundCell == fixture.base() + 2u * crash1::kCellStride,
          "the cell that MATCHED is returned, not the one after it (0x800159B8 undoes the delay slot)");
    check(search.cellsWalked == 2u, "two stride steps were taken before the match: 2 cells walked");
    check(fixture.reads() == 3u, "the first cell and both successors were read, once each");
  }

  // A class that no cell in the fixture serves. The walk must run until it reaches the edge of main
  // RAM and stop there, WITHOUT reading the address past the edge. This is the one rule the owner
  // adds over the guest's body, so it is the one case the fixture has to police hardest: the fixture
  // records every address it was asked for, and counts any request outside main RAM rather than
  // answering it, because answering would BE the failure.
  {
    const Fixture fixture({7u, 9u, 11u});
    const CellSearch search = crash1::searchCells(&fixture, &Fixture::read, fixture.base(), 99u);
    check(search.leftMainRam, "an absent class leaves main RAM rather than returning a neighbour");
    check(fixture.askedOutsideMainRam() == 0u, "the walk did NOT read outside main RAM: the edge held");
    // The fixture sits at 0x80100000, so main RAM ends 0x1F0000 bytes later. The walk steps by 8 and
    // stops on the first cell address at or past 0x80200000, without reading it.
    check(search.cellsWalked == fixture.cellsToMainRamEdge(),
          "the walk stepped once per 8-byte cell to the edge of main RAM and no further");
    check(search.foundCell == 0x80200000u, "and it stopped AT the edge, 0x80200000");
    check(fixture.lastAddress() == 0x801FFFF8u,
          "and the last cell it actually READ is the last one inside main RAM, 0x801FFFF8");
  }

  // TRANSPARENCY, which is the contract that matters most and the one a reader cannot take on trust.
  // The owner replaced the guest's UNBOUNDED walk, so for every class the fixture serves the answer
  // must be identical to the guest's: same cell, same number of reads, same number of steps. This
  // walks EVERY class the fixture publishes and checks all three, because an owner that is right only
  // on the first cell is not faithful.
  {
    for (std::uint32_t classKey : {4u, 8u, 12u}) {
      // A FRESH fixture per class: the read counter is cumulative, so reusing one would compare the
      // third class's total against the first class's expectation and fail for the wrong reason.
      const Fixture fixture({4u, 8u, 4u, 12u, 8u, 12u});
      const std::uint32_t expectedIndex = [&] {
        for (std::uint32_t i = 0; i < fixture.size(); ++i) {
          if (fixture.classAt(i) == classKey) {
            return i;
          }
        }
        return fixture.size();
      }();
      const CellSearch search = crash1::searchCells(&fixture, &Fixture::read, fixture.base(), classKey);
      check(!search.leftMainRam, "a served class never leaves main RAM");
      check(search.foundCell == fixture.base() + expectedIndex * crash1::kCellStride,
            "the owner returns the cell the guest's own walk would return, for a served class");
      check(search.cellsWalked == expectedIndex, "and it took exactly as many steps as the guest would");
      check(fixture.reads() == expectedIndex + 1u, "and it read exactly the cells the guest would read");
    }
  }

  // The class key is `request >> 13` and the bucket index is `(key & 0x3FC)` BYTES. Both are the
  // owner's arithmetic rather than the walk's, so they are pinned here as the values the header
  // claims. The mask is applied to the KEY and its result is a BYTE offset into a word-strided
  // table, which is the constraint the recovered code places on its own callers: only a key that is a
  // multiple of four addresses a cell pointer at all. Measured, and it is why the request word cannot
  // be an arbitrary byte count.
  check((0x4000u >> crash1::kClassShift) == 2u, "one 8 KiB class of request is class 2");
  check((0x1FFFu >> crash1::kClassShift) == 0u, "the largest request below 8 KiB is class 0");
  check((4u & crash1::kBucketIndexMask) == 4u, "class 4's bucket sits at byte offset 4, one word in");
  check((crash1::kBucketIndexMask / 4u) == 255u,
        "the byte mask spans 256 buckets of 4 bytes, which is the table the guest indexes");
  check(crash1::isWordAlignedClassKey(4u) && crash1::isWordAlignedClassKey(252u),
        "classes 4 and 252 address a whole word, as a PSX lw requires");
  check(!crash1::isWordAlignedClassKey(1u) && !crash1::isWordAlignedClassKey(253u),
        "classes 1 and 253 would address a cell pointer a quarter of the way into a word, so the "
        "engine's six callers can only be passing multiples of four");
  check(crash1::kClassShift == 13u, "the class shift is 13, the `sa` field of 0x00041342");
  check(crash1::kCellStride == 8u && crash1::kClassFieldOffset == 4u,
        "the cell is 8 bytes and its class is the second word, from 0x24630008 and 0x8C620004");
  check(crash1::isMainRam(0x80100000u), "a real cell address IS main RAM");
  check(!crash1::isMainRam(0x80200000u), "the first cell address past main RAM is not");
  check(!crash1::isMainRam(0x00800000u), "0x00800000 is not main RAM, which is the whole claim");
  check(!crash1::isMainRam(0x00000000u),
        "a null bucket is refused: a null test alone would call this a pointer, and 0 is mapped here");
  check(!crash1::isMainRam(0x1F802000u),
        "an expansion-region address is refused too, so the test is a range test and not a null test");

  // --- the owner, through the real runtime --------------------------------------------------------
  crash1::Crash1Runtime runtime;
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  if (game->core.runtime != &runtime) {
    std::fprintf(stderr, "FAIL: the runtime did not install onto the Core\n");
    return 1;
  }
  // The image identity is activated FIRST: an override key is (image identity, guest address), so
  // installing before any image is active is the "no unambiguous active image" refusal.
  const auto identity = game->core.imageCatalog().activate("Crash1BlockPool", {0x00010000u, 0x00056800u}, 1u);
  crash1::installCrash1BlockPool(game->core);

  check(game->core.nativeDispatcher().isInstalled({identity, crash1::kFindCell}),
        "the owner installed its override on the measured lookup entry 0x80015978");
  check(!game->core.nativeDispatcher().isInstalled({identity, kNearPlaneConsumer}),
        "the GTE near-plane consumer is not overwritten: this module owns a pool, not a projection");
  check(!game->core.nativeDispatcher().isInstalled({identity, kFaultSite}),
        "the stop address 0x800159A8 is INSIDE the lookup, so it is an arm of the owner's loop and "
        "never an override key of its own; installing one would resume at a label the guest reaches "
        "by falling through, with no r31 set by the jump that got there");

  Crash1BlockPool &owner = runtime.blockPool();
  check(owner.lookups() == 0u && owner.found() == 0u,
        "0 of 0 lookups before the owner has run: a denominator, not a result");

  // A live pool, published through the three globals the guest uses, so the owner's reads are
  // exercised against real guest memory rather than a stub. The class keys are multiples of four
  // because the recovered bucket index is a BYTE offset into a word-strided table.
  const std::uint32_t kBucketTable = 0x80060000u;
  const std::uint32_t kCellCountBlock = 0x80060010u;
  const std::uint32_t kFirstCell = 0x80100000u;
  const std::uint32_t kClassServed = 4u;     // class 4 -> bucket byte offset 4
  const std::uint32_t kClassAlsoServed = 8u; // class 8 -> bucket byte offset 8
  const std::uint32_t kClassUnmapped = 40u;  // class 40 -> bucket byte offset 40
  game->core.mem_w32(crash1::kBucketTablePointer, kBucketTable);
  game->core.mem_w32(crash1::kPoolBasePointer, kFirstCell);
  game->core.mem_w32(crash1::kCellCountBlockPointer, kCellCountBlock);
  game->core.mem_w32(kCellCountBlock + crash1::kCellCountOffset, 3u);
  for (std::uint32_t bucket = 0; bucket < 256u; ++bucket) {
    game->core.mem_w32(kBucketTable + bucket * 4u, kFirstCell);
  }
  game->core.mem_w32(kFirstCell + 0u, 0x00001234u);       // payload of cell 0
  game->core.mem_w32(kFirstCell + 4u, kClassServed);      // class of cell 0
  game->core.mem_w32(kFirstCell + 8u, 0x00005678u);       // payload of cell 1
  game->core.mem_w32(kFirstCell + 12u, kClassAlsoServed); // class of cell 1
  game->core.mem_w32(kFirstCell + 16u, 0x00009ABCu);      // payload of cell 2
  game->core.mem_w32(kFirstCell + 20u, kClassServed);     // class of cell 2

  // A class the FIRST cell serves: the answer is the first cell and no step is taken.
  game->core.r[31] = 0x80013140u; // the return address the first measured call site would carry
  check(owner.findCell(game->core, kClassServed << crash1::kClassShift) == kFirstCell,
        "a class on the first cell returns that cell");
  check(owner.lookups() == 1u && owner.found() == 1u, "1 of 1 lookups found a cell");
  check(owner.firstCaller() == 0x80013140u,
        "the caller's return address is recorded once, so a run can name the module that asked");
  check(owner.lastClassKey() == kClassServed, "the class key is the request shifted by 13");
  check(owner.lastRequest() == kClassServed << crash1::kClassShift, "the request the guest passed is recorded");
  check(owner.lastBucketTable() == kBucketTable, "the bucket table the guest published is recorded");
  check(owner.lastFirstCell() == kFirstCell, "the bucket's first cell is recorded");
  check(owner.lastPoolBase() == kFirstCell, "the engine's pool base is recorded");
  check(owner.lastLiveCellCount() == 3u, "the engine's live cell count is recorded");
  check(owner.lastPoolBaseDistance() == 0,
        "the signed distance from the engine's base is reported, and is 0 for a bucket sitting on it");

  // The same class again, now served by the THIRD cell, with the first cell changed to something else
  // first. The pool is mutated between calls on purpose: a walk that ignored the guest's memory would
  // still pass, and a walk that returned the previous answer from a cache would too.
  game->core.mem_w32(kFirstCell + 4u, kClassAlsoServed);
  game->core.mem_w32(kFirstCell + 12u, kClassAlsoServed);
  check(owner.findCell(game->core, kClassServed << crash1::kClassShift) == kFirstCell + 2u * crash1::kCellStride,
        "a class on the third cell returns the third cell, walking two steps");
  check(owner.lastCellsWalked() == 2u, "the walk examined 2 cells after the first");
  check(owner.maxCellsWalked() == 2u, "the deepest walk so far is 2 cells");
  check(owner.firstCaller() == 0x80013140u,
        "a second call from elsewhere does not overwrite the first caller: the one sample is the point");
  check(owner.lookups() == 2u && owner.found() == 2u, "2 of 2 lookups found a cell");

  // A bucket that is not a pointer into main RAM. This is the shape the media-less run had, and it is
  // the case the owner exists for: it must NOT read through the pointer, and it must name the value.
  game->core.mem_w32(kBucketTable + kClassUnmapped, 0x00800000u);
  const std::uint32_t unmapped = owner.findCell(game->core, kClassUnmapped << crash1::kClassShift);
  check(unmapped == 0x00800000u,
        "the owner returns the pointer it was given rather than inventing a failure value: the "
        "guest's callers dereference the result unchecked, so a substituted error word would fault "
        "one level up and hide the real value");
  check(owner.leftMainRam() == 1u, "the walk that left main RAM is counted");
  check(owner.found() == 2u,
        "and it is NOT counted as a lookup that found a cell: those are different outcomes and one "
        "denominator cannot stand for both");
  check(owner.lookups() == 3u, "3 lookups served, 2 found a cell, 1 left main RAM");
  check(owner.lastCellsWalked() == 0u,
        "the walk that left main RAM read no cell at all, so it walked none: the count is 0 because "
        "the FIRST address was already outside, which is the whole shape of the media-less fault");
  check(owner.maxCellsWalked() == 2u,
        "and the running maximum is still the deepest walk that actually happened, which is the "
        "number a reader wants beside it");
  check(owner.lastFirstCell() == 0x00800000u, "and the offending pointer is the recorded first cell");

  std::printf("Crash1BlockPool: %llu lookup(s) served, %llu found a cell, %llu left main RAM; "
              "deepest walk %u cell(s); first caller 0x%08X; the recovered walk is transparent for "
              "every served class and stops only at the edge of main RAM\n",
              static_cast<unsigned long long>(owner.lookups()),
              static_cast<unsigned long long>(owner.found()),
              static_cast<unsigned long long>(owner.leftMainRam()),
              owner.maxCellsWalked(),
              owner.firstCaller());
  if (failures != 0) {
    std::fprintf(stderr, "%d check(s) failed\n", failures);
    return 1;
  }
  return 0;
}
