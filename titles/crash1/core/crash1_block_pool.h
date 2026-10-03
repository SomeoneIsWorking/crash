// Crash Bandicoot 1 (SCUS-949.00) - the title's own SIZE-CLASS BLOCK POOL owner.
//
// The guest PC the product stopped on was 0x800159A8, `lw $v0,0x4($v1)` (0x8C620004), the second
// class-field read of the engine's block-cell lookup at 0x80015978. This file is that function,
// recovered, and `crash1_block_pool.cpp` registers it as a native override so the JIT no longer
// guesses at it. The JIT still runs everything else.
//
// The recovered body, instruction by instruction:
//
//   0x80015978  srl  v0,a0,13          ; the BUCKET INDEX source is the request shifted down 13
//   0x8001597C  lui  v1,0x8006
//   0x80015980  lw   v1,-0x3AD0(v1)    ; v1 = *(0x8005C530): the base of a 256-entry bucket table
//   0x80015984  andi v0,v0,0x03FC      ; ... indexed by (key & 0x3FC) BYTES, so 4 bytes per bucket
//   0x80015988  addu v0,v0,v1
//   0x8001598C  lw   v1,0(v0)          ; v1 = the first cell of that bucket
//   0x80015994  lw   v0,4(v1)          ; does the first cell serve this class?
//   0x8001599C  beq  v0,a0,0x800159BC   ; yes -> return it.  THE COMPARISON IS AGAINST $a0 ITSELF.
//   0x800159A4  addiu v1,v1,8           ; no -> step one cell
//   0x800159A8  lw   v0,4(v1)          ; <-- THE STOP. No bound is checked anywhere on this path.
//   0x800159B0  bne  v0,a0,0x800159A8   ; ... and the back edge targets 0x800159A8, the loop head
//   0x800159B4  addiu v1,v1,8           ; delay slot, always
//   0x800159B8  addiu v1,v1,-8          ; what the delay slot is undone by on the match path
//   0x800159BC  jr   ra
//   0x800159C0  addu v0,v1,zero
//
// so the body is exactly:
//
//   key   = request >> 13                        // the shift picks the BUCKET, and nothing else
//   cell  = bucket[(key & 0x3FC) >> 2]
//   if (cell->class == request) return cell;      // <-- the WHOLE request word, not `key`
//   for (cell += 1; cell->class != request; cell += 1) {}
//   return cell;
//
// THE CLASS IS THE REQUEST WORD ITSELF, and the shift is only a bucket selector. That is three
// branch instructions in this image whose second operand is register `$a0`, which nothing between the
// entry and each branch redefines (0x8001599C, 0x800159B0, and 0x800159E8 in the bounded sibling),
// and the pool's own allocate path compares the SAME unshifted word. An owner that compared the
// shifted key against the cell's class could never match a real cell, its walk would never end, and
// it would hand the caller a cell at the edge of main RAM. `titles/crash1/executable.json` records
// the comparison register of all three class branches.
//
// The only difference between a lookup that returns and one that faults is whether the bucket holds a
// real cell pointer; nothing else in the function can fault.
//
// THE ENGINE'S OWN BOUND IS A MEASUREMENT, NOT A RULE. The same image carries this same computation
// WITH a bound at 0x800159C4: it reads 0x8005C534 as a pool base, takes the live cell count from
// `*(0x8005C540) + 0x404`, rejects when `((cell - poolBase) >> 3) >= count`, and returns
// 0xFFFFFFF6. It has ZERO `jal` call sites, so it does not run. Applying that bound was measured to
// be wrong: a controlled run of the same product and disc with the bound applied reported "no cell
// serving class 702 within 576 live cell(s)" and faulted, while the same run with the bound removed
// reached the game's first measured display wait. The guest's 256 buckets are not one contiguous
// array, so a bucket outside the array 0x8005C534 describes is rejected on its first step even
// though its class is serviceable. The bound is therefore read, evaluated and REPORTED per lookup,
// but it does not decide.
//
// What decides instead is a HOST-MEMORY fact: this owner's walk reads a cell only while the cell's
// address is in the guest's own 2 MiB of main RAM, which is true for every cell retail can read and
// false for every cell retail would fault on. That cannot change the answer for any case retail
// executes, and it turns the one case retail cannot execute into a named refusal with the value the
// guest published, instead of a read at 0x00800004.
//
// WHAT IS NOT ESTABLISHED, because a recovery that overstates itself is worse than none. The UNIT of
// `request` is unknown: the four words measured from the live run are not byte counts, not sizes and
// not handles this repository can name, so the field is named for what it is compared against and no
// more. The six `jal 0x80015978` call sites are equally reachable from CoreLoop, so a call-graph
// census cannot rank them; `Crash1BlockPool::firstCaller()` is the answer. And this is not a claim
// that the pool is the ROOT CAUSE of Crash 1 presenting no frame: it is a claim that this is the code
// that faulted, that it is now readable and native, and that the faulting state is reported with its
// value.
#pragma once

#include <cstddef>
#include <cstdint>

class Core;

namespace crash1 {

// The two functions, by address. Same pattern as `crash1_horizontal_bound.h`: the literal lives here
// and the authority is titles/crash1/executable.json.
inline constexpr std::uint32_t kFindCell = 0x80015978u;
inline constexpr std::uint32_t kFindCellEnd = 0x800159C4u;
inline constexpr std::uint32_t kFindCellBounded = 0x800159C4u;
inline constexpr std::uint32_t kFindCellBoundedEnd = 0x80015A3Cu;

// The recovery's arithmetic, each value decoded out of the instruction that produces it.
//   bucket-index shift 0x80015978  0x00041342  srl v0,a0,13   (a0>>13: the BUCKET selector, nothing more)
//   bucket index mask  0x80015984  0x304203FC  andi v0,v0,0x03FC   (a byte offset, 4 per bucket)
//   cell stride        0x800159A4  0x24630008  addiu v1,v1,8       (and again at 0x800159B4)
//   class field        0x80015994  0x8C620004  lw v0,4(v1)        (half the stride)
// The class a cell is compared against is the request word UNCHANGED; see the header's decoded body.
inline constexpr std::uint32_t kClassShift = 13u;
inline constexpr std::uint32_t kBucketIndexMask = 0x03FCu;
inline constexpr std::uint32_t kCellStride = 8u;
inline constexpr std::uint32_t kClassFieldOffset = 4u;

// The pool's three globals. Each is reached by a `lui $rX,0x8006` + `lw $rY,disp($rX)` pair, so none
// carries its address as an immediate anywhere and a literal-immediate scan cannot see them. None of
// them has a `sw` at that displacement either: the pool is filled through a computed pointer, which is
// why "who initialised it" is a separate question from "what is in it".
inline constexpr std::uint32_t kBucketTablePointer = 0x8005C530u;    // -> 256 bucket cell pointers
inline constexpr std::uint32_t kPoolBasePointer = 0x8005C534u;       // -> a pool base, for the bound form
inline constexpr std::uint32_t kCellCountBlockPointer = 0x8005C540u; // -> a block, +0x404 is the count
inline constexpr std::uint32_t kCellCountOffset = 0x404u;

// The engine's own "no cell serves this class" result, from the bounded sibling at 0x800159C4:
// `addiu v0,zero,-10` at 0x80015A18. Recorded because the guest's callers do NOT check it —
// 0x80015174 dereferences whatever the lookup returns — so returning it here would fault one level up
// rather than answer. It is a fact about the image, not a value this owner hands back.
inline constexpr std::int32_t kPoolExhausted = -10;

// The stop, pinned by its own word rather than by a range digest, because this address is the whole
// claim and a range that merely hashed correctly would not say what is at it.
inline constexpr std::uint32_t kFaultSite = 0x800159A8u;

// One 8-byte cell of the pool. `payload` is what the allocate path stores at 0x80013020
// (`sw a1,0(s3)`) and what the release path reads back at `lw a0,0(s0)`; `cellClass` is the only
// field this module compares. Declared as a struct so the recovered code names the recovered layout
// instead of doing pointer arithmetic on a raw word — and it is only ever a DESCRIPTION: the guest
// bytes are read through `CellClassReader`, never dereferenced through this type, because guest RAM
// is a byte array behind an accessor and aliasing it as a C++ object would be a lie about the host.
struct BlockCell {
  std::uint32_t payload;
  std::uint32_t cellClass; // the request word the engine compares at 0x8001599C, unshifted
};
static_assert(sizeof(BlockCell) == kCellStride, "the measured cell stride is the recovered cell size");
static_assert(offsetof(BlockCell, cellClass) == kClassFieldOffset,
              "the measured class field is the recovered cell's second word");

// Reads one word out of a guest cell. Injected so the recovered walk can be pinned against a
// fixture with no Core in existence, and so the shipping owner reads guest memory through the one
// accessor the framework provides rather than through a cast of a guest address.
using CellClassReader = std::uint32_t (*)(const void *context, std::uint32_t cell);

// The recovered walk. Every quantity is an ADDRESS or a COUNT, never a host pointer, so the same
// function serves the owner and the test. `request` below is the WHOLE word: the shift at 0x80015978
// selects a bucket and never reaches the comparison at 0x8001599C.
struct CellSearch {
  std::uint32_t foundCell;   // the cell whose class matched; meaningless when `leftMainRam` is set
  std::uint32_t cellsWalked; // cells examined; the denominator a reader needs to judge a walk
  bool leftMainRam;          // the next cell was outside the guest's 2 MiB of main RAM
};

// `0x80015994` tests the first cell before the loop; `0x800159A8` is the loop head and tests every
// cell after it. `0x800159B4`'s delay slot steps unconditionally and `0x800159B8` gives one step back
// on the match path, so a match returns the cell that matched and not its successor. This is that
// control flow and nothing else.
//
// THE STOPPING RULE IS MAIN RAM, NOT THE ENGINE'S POOL BOUND, and the file header says why at
// length: the pool bound from 0x800159C4 was applied, measured to break a path retail completes, and
// removed. `isMainRam` is a range test rather than a null test on purpose — a null test would pass
// for the low addresses that ARE mapped on this machine, and would call a merely-wrong pointer
// "uninitialised", which is the specific misreading this owner exists to avoid.
[[nodiscard]] CellSearch
searchCells(const void *context, const CellClassReader &read, std::uint32_t firstCell, std::uint32_t request) noexcept;

// Main RAM is 0x80000000..0x801FFFFF on this machine: 2 MiB, mirrored through KSEG0 and KSEG1. A
// pool cell lives there, so this is the one range test that says whether a published cell pointer can
// be read at all.
[[nodiscard]] constexpr bool isMainRam(std::uint32_t address) noexcept {
  return (address & 0xFFE00000u) == 0x80000000u;
}

// The bucket index is `((request >> 13) & 0x3FC)` BYTES into a table whose entries are one word each,
// so a shifted key that is not a multiple of 4 would address a cell pointer a quarter of the way into
// a word. The recovered function does not care — it is a byte add and a load — but the PSX `lw` does,
// so the engine can only ever call it with a request whose bits 13 and up form a multiple of 4. That
// is a property of the six CALLERS, established here because it constrains what the request may be.
[[nodiscard]] constexpr bool isWordAlignedClassKey(std::uint32_t request) noexcept {
  return ((request >> kClassShift) & 0x3u) == 0u;
}

// This title's owner. It records what each lookup saw, so a run can say WHICH condition was false
// rather than only that the guest stopped.
class Crash1BlockPool final {
public:
  // What the last lookup searched for and what the guest had published when it did. A run reporting
  // a stop address without these is reporting a symptom.
  [[nodiscard]] std::uint32_t lastRequest() const {
    return lastRequest_;
  }
  [[nodiscard]] std::uint32_t lastClassKey() const {
    return lastClassKey_;
  }
  [[nodiscard]] std::uint32_t lastBucketTable() const {
    return lastBucketTable_;
  }
  [[nodiscard]] std::uint32_t lastFirstCell() const {
    return lastFirstCell_;
  }
  [[nodiscard]] std::uint32_t lastPoolBase() const {
    return lastPoolBase_;
  }
  [[nodiscard]] std::uint32_t lastLiveCellCount() const {
    return lastLiveCellCount_;
  }
  [[nodiscard]] std::uint32_t lastCellsWalked() const {
    return lastCellsWalked_;
  }

  // The engine's own bound, EVALUATED for the last lookup and reported rather than enforced. The
  // distance is signed, because the guest's `sra`/`slt` pair is, and it is negative for a bucket below
  // the base 0x8005C534 publishes. A reader needs this to see WHY the bound was not used as a rule.
  [[nodiscard]] std::int32_t lastPoolBaseDistance() const {
    return lastPoolBaseDistance_;
  }

  // Denominators. "0 of 0 lookups found a cell" is an instrument that never ran, not a fact about
  // the pool, so every report of this owner has to carry them.
  [[nodiscard]] std::uint64_t lookups() const {
    return lookups_;
  }
  [[nodiscard]] std::uint64_t found() const {
    return found_;
  }
  [[nodiscard]] std::uint64_t leftMainRam() const {
    return leftMainRam_;
  }
  [[nodiscard]] std::uint32_t maxCellsWalked() const {
    return maxCellsWalked_;
  }

  // The return address the first time a lookup was served, so a run can name the module that asked.
  // Recorded once: a per-call sample of something that runs thousands of times a frame would drown
  // the one answer wanted. Zero until a lookup has run.
  [[nodiscard]] std::uint32_t firstCaller() const {
    return firstCaller_;
  }

  // Whether this run has already reported its first lookup and its first served cell. A run needs to
  // say "1 of N found a cell" and NOT only "0 left main RAM": the second is a count of failures with
  // no denominator beside it, and "no errors appeared in this log" is exactly the sentence that
  // reads as a pass when nothing ran at all.
  [[nodiscard]] bool reportedFirstLookup() const {
    return reportedFirstLookup_;
  }
  [[nodiscard]] bool reportedFirstFound() const {
    return reportedFirstFound_;
  }

  // The recovered lookup, executed against guest memory. This is the whole of the override.
  [[nodiscard]] std::uint32_t findCell(Core &core, std::uint32_t request);

  // This title's owner, reached from a Core that is running it. Same idiom and same reason as
  // `Crash1HorizontalBound::from`.
  static Crash1BlockPool &from(Core &core);

private:
  std::uint32_t lastRequest_{};
  std::uint32_t lastClassKey_{};
  std::uint32_t lastBucketTable_{};
  std::uint32_t lastFirstCell_{};
  std::uint32_t lastPoolBase_{};
  std::uint32_t lastLiveCellCount_{};
  std::uint32_t lastCellsWalked_{};
  std::int32_t lastPoolBaseDistance_{};
  std::uint32_t maxCellsWalked_{};
  std::uint32_t firstCaller_{};
  bool reportedFirstLookup_{};
  bool reportedFirstFound_{};
  std::uint64_t lookups_{};
  std::uint64_t found_{};
  std::uint64_t leftMainRam_{};
};

// Install this title's block-pool owner on one Core.
void installCrash1BlockPool(Core &core);

} // namespace crash1
