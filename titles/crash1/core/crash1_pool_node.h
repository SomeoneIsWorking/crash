// Crash Bandicoot 1 (SCUS-949.00) — the pool's OWN ALLOCATE PATH, recovered from 0x80012F10.
//
// WHY THIS FILE EXISTS, and the address it recovers. The size-class lookup at 0x80015978
// (`crash1_block_pool.h`) only READS the pool. This is the function that FILLS it, and the reason it
// matters is that it is where the class word is DEFINED: 0x80012FBC loads the word, 0x80012FC8
// shifts it down 13 to pick a bucket, and 0x80012FFC loads the SAME unshifted word back out to
// compare against a cell's class field. A lookup owner that had searched for the shifted key could
// therefore never match anything this function wrote — which is exactly the defect measured and
// corrected in `crash1_block_pool.h`.
//
// It has two `jal` call sites:
//   0x8001313C  inside 0x80012F10   <- this function, calling the lookup it is building a cell for
//   0x8001515C  inside 0x80015118   <- the caller every live lookup in the disc-backed run came from
// and the first of those is the loop that closes: 0x80013134 loads the class word, 0x80013138 loads
// the request, 0x8001313C calls the lookup, and 0x80013140 (the delay slot) STORES the class word
// into the cell. **The engine stores the word it is about to ask for.** That is a fourth,
// independent reason the class is the whole request and not `request >> 13`.
//
// WHAT THIS OWNER IS, EXACTLY, because "recovered" is a word that has been abused in this
// repository. It is a READABLE, TESTED MODEL of the function's arithmetic: the node-table cursor, the
// five-way kind dispatch, the bucket selection, the class comparison, the cell link, the live counter,
// and the per-type callback dispatch. Every guest access goes through an injected seam, so the model
// is driven by a fixture with no `Core` in existence.
//
// WHAT THIS OWNER IS NOT, stated here so nobody reads it as a result. It is NOT installed as a native
// override, and this file claims no runtime behaviour from it. Two reasons, and the first is the
// honest one: the function is 259 instructions across five cases and makes THREE guest calls (the
// lookup at 0x80015978, 0x8001439C, and 0x80040484) plus an indirect `jalr` through a table of
// function pointers, so a faithful override is a real porting job rather than a transcription. The
// second is a measurement: in the disc-backed run the pool owner served lookups and every one of them
// came from `ra = 0x80015164` (FUN_80015118), so nothing yet establishes that the guest REACHES
// 0x80012F10 at all. Owning a function the run does not execute would be a claim with no evidence
// behind it, and the point of this file is to make the next step a measurement rather than a guess.
//
// WHAT IS NOT ESTABLISHED. The UNIT of the class word is still unknown. It is a 32-bit word whose top
// 19 bits select one of 256 buckets and whose low 13 bits are part of the identity; the four words
// measured from the live run (0x0057CCFB, 0x15814CE7, 0x5452D94D, 0x4E938CCD) are not byte counts,
// not sizes and not handles this repository can name. The model therefore calls the field for what it
// is COMPARED against and for nothing more.
#pragma once

#include <cstdint>

namespace crash1::pool_node {

// The function, by address. Same rule as every other owner in this title: the literal lives here and
// the authority is titles/crash1/executable.json.
inline constexpr std::uint32_t kEntry = 0x80012F10u;
inline constexpr std::uint32_t kEnd = 0x8001331Cu; // the `jr $ra` at 0x80013314, plus its delay slot

// THE FOUR CALL SITES, and each one's role. The first is the loop this function closes by calling
// the lookup it just wrote a cell for; the second is the live caller in the disc-backed run. Six
// sites reach the lookup in total and the other four are in other functions.
inline constexpr std::uint32_t kLookupCall = 0x80015978u;
inline constexpr std::uint32_t kCallToLookup = 0x8001313Cu; // ra = 0x80013140
inline constexpr std::uint32_t kCallToLookupDelaySlot = 0x80013140u;
inline constexpr std::uint32_t kMeasureDeadEndCall = 0x8001439Cu; // the kind-1 path's only call
inline constexpr std::uint32_t kEscapeClassInit = 0x80040484u;    // the kind-2 path's only call
inline constexpr std::uint32_t kLiveCallerCall = 0x8001515Cu;     // ra = 0x80015164, the measured caller

// The node table and its cursor. None of these is reached by a literal address in the image: each is
// a `lui $rX,0x8006` + `addiu`/`lw` pair, which is why a literal-immediate scan cannot see them and
// why this list is re-derived from the decoded words rather than searched for.
inline constexpr std::uint32_t kNodeTable = 0x8005C554u;         // lui 0x8006 + addiu -0x3AAC  (0x80012F34)
inline constexpr std::uint32_t kNodeCursor = 0x8005CFACu;        // lui 0x8006 + lw    -0x3054  (0x80012F2C)
inline constexpr std::uint32_t kBucketTable = 0x8005C530u;       // lui 0x8006 + lw    -0x3AD0  (0x80012FC4)
inline constexpr std::uint32_t kHandleTable = 0x800580A0u;       // lui 0x8006 + addiu -0x7F60  (0x800130D8)
inline constexpr std::uint32_t kTypeCallbackTable = 0x800514ECu; // lui 0x8005 + lw +0x14EC    (0x80013050)
inline constexpr std::uint32_t kEscapeClassTable = 0x8005728Cu;  // lui 0x8005 + addiu 0x728C  (0x800131BC)

// The two strides, each decoded out of the arithmetic that produces it rather than asserted.
//   node stride     a0*2 + a0 = 3a; 3a*4 = 12a; 12a - a = 11a; 11a*4 = 44a   (0x80012F14..0x80012F24)
//   callback stride n*8 - n = 7n; 7n*4 = 28n                                  (0x8001303C..0x80013044)
// The escape-class entry stride is the same 44 the node table uses (0x800130C0..0x800130D0 computes
// the identical 44a), which is why one constant names both and the probe checks both sites.
inline constexpr std::uint32_t kNodeStride = 44u;
inline constexpr std::uint32_t kCallbackStride = 28u;

// The bucket selector, which is the SAME arithmetic the lookup at 0x80015978 uses and is declared in
// `crash1_block_pool.h`. It is redeclared rather than included because these two owners are
// independently gated by two independently failing probes, and a shared header would make one wrong
// constant fail in a place that does not own it.
inline constexpr std::uint32_t kBucketIndexShift = 13u;
inline constexpr std::uint32_t kBucketIndexMask = 0x03FCu;
inline constexpr std::uint32_t kCellStride = 8u;

// The class word. `srl $2,$2,13` at 0x80012FC8 produces the BUCKET and nothing else, exactly as at
// 0x80015978; the word itself is what a cell's class field is compared against, and what 0x80013140
// stores into the cell. See the header comment.
inline constexpr std::uint32_t kClassWordOffset = 0x14u;  // node+0x14, stored into the cell at 0x80013140
inline constexpr std::uint32_t kRequestOffset = 0x18u;    // cell+0x18, the word the lookup is called with
inline constexpr std::uint32_t kNodeElementArray = 0x10u; // node+0x10, the per-node element list
inline constexpr std::uint32_t kNodeLimit = 0x08u;        // node+0x08, how many elements to service
inline constexpr std::uint32_t kNodeKind = 0x02u;         // node+0x02, halfword, the five-way dispatch
inline constexpr std::uint32_t kNodePayload = 0x00u;      // node+0x00, the node body pointer
inline constexpr std::uint32_t kNodeLiveCount = 0x0Au;    // node+0x0A, halfword, ++/-- around a call
inline constexpr std::uint32_t kNodeState = 0x04u;        // node+0x04, halfword, published as 0x1E
inline constexpr std::uint32_t kNodeContext = 0x24u;      // node+0x24, the kind-3/4 context
inline constexpr std::uint32_t kElementKind = 0x08u;      // element+0x08, indexes the callback table
inline constexpr std::uint32_t kElementType = 0x10u;      // element+0x10, the kind-3/4 packed type

// The five-way dispatch, from the comparisons at 0x80012F74..0x80012FA4. `kKindOther` is not a
// handler: 0x800132E8 is the common tail that republishes the node state as 0x14 and returns.
inline constexpr std::uint32_t kDispatchKind = 0x80012F70u;
inline constexpr std::uint32_t kKindZero = 0x80012FB4u;
inline constexpr std::uint32_t kKindOne = 0x80013094u;
inline constexpr std::uint32_t kKindTwo = 0x800130C0u;
inline constexpr std::uint32_t kKindThreeOrFour = 0x8001317Cu;
inline constexpr std::uint32_t kKindOther = 0x800132E8u;

// The sentinel 0x8001439C returns, from `addiu $2,$zero,-0xC` at 0x800130A0, and the one it is
// compared against at 0x800130A4. A node whose measure returns this takes the kind-1 shortcut.
inline constexpr std::int32_t kMeasureDeadEnd = -12;

// Published state words, each with the instruction that writes it: `ori $2,$zero,0x1E` at
// 0x80012F68 (node+0x04 = 30), `ori $2,$zero,0x14` at 0x800132EC (node+0x04 = 20), and
// `ori $3,$zero,0x1` at 0x800132BC (node+0x04 = 1).
inline constexpr std::uint16_t kStateBuilt = 0x1Eu;
inline constexpr std::uint16_t kStateDone = 0x14u;
inline constexpr std::uint16_t kStateReset = 0x1u;

// The class word the lookup is asked for, and the byte at which the engine stores the class word into
// the cell. `CellLink` is the one piece of this function the LOOKUP owner also depends on, so it is a
// named function here and is tested against the same whole-word rule the lookup uses.
struct CellLink {
  std::uint32_t cell;         // the cell that carries the class word
  std::uint32_t classWord;    // the WHOLE word, not the bucket key
  std::uint32_t bucketOffset; // (classWord >> 13) & 0x3FC
  std::uint32_t cellsWalked;  // the denominator: 0 means the FIRST cell already carried it
  bool exhausted;             // the walk reached the edge of the bucket without a match
};

// The two seams. Memory is read and written through these, so the model is a model of the ARITHMETIC
// and not a second guest-memory implementation: the shipping caller supplies a Core-backed reader.
struct Memory {
  const void *context;
  std::uint32_t (*read)(const void *context, std::uint32_t address) noexcept;
  void (*write)(const void *context, std::uint32_t address, std::uint32_t value) noexcept;
};

// THE KIND-0 LOOP, 0x80012FB4..0x80013090, and the only part of this function that writes pool
// cells. Recovered body:
//
//   word     = *(node + 0x10) + 4        // 0x80012FB4 `lw $2,0x10($4)`; 0x80012FBC `lw $2,4($2)`
//   bucket   = *(0x8005C530) + ((word >> 13) & 0x3FC)
//   first    = *bucket                   // 0x80012FD8
//   if (*(node + 0x08) == 0) return;    // 0x80012FDC, the empty node
//   linked = 0;
//   do {
//     element = *(node + 0x10 + linked * 4);      // 0x80012FF4
//     class   = element->class;                  // 0x80012FFC, the WHOLE word again
//     while (cell->class != class) cell += 1;    // 0x8001300C..0x80013018, unbounded
//     cell->payload = element;                   // 0x80013020 `sw $5,0($19)` — THE CELL WRITE
//     ++*(node + 0x0A);                          // 0x80013024..0x80013030
//     if (*(0x800514EC + element->type * 28)) { ++linked; callback(element); }
//     --*(node + 0x0A);                          // 0x80013068..0x80013074
//   } while (linked < *(node + 0x08));           // 0x80013078..0x80013084
//
// The `word` above is the SAME value as `class` in the loop body — 0x80012FF4 loads
// `node->elements[linked]` and 0x80012FFC loads its `+4`, which is the word 0x80012FBC already read
// for the bucket. The engine uses one word for two purposes, and a transcription that treated the
// shifted value as the class would break the second purpose.
[[nodiscard]] CellLink linkFirstCell(const Memory &memory, std::uint32_t node, std::uint32_t classWord) noexcept;

// The bucket the class word selects, from 0x80012FC8..0x80012FD0. Split out because it is the single
// arithmetic the lookup and this function SHARE, and a divergence between them is exactly the class
// of bug this recovery exists to prevent.
[[nodiscard]] std::uint32_t bucketAddress(const Memory &memory, std::uint32_t classWord) noexcept;

// The node the cursor at 0x8005CFAC names, from 0x80012F14..0x80012F3C, INCLUDING the reset: the
// function compares the published cursor against the node it just computed and zeroes the cursor when
// they are equal (0x80012F54 `bne`, 0x80012F5C `lui`, 0x80012F60 `sw $zero,-0x3054($at)`). A model
// that omitted the reset would hand out the same node twice, so the test drives the equal case.
[[nodiscard]] std::uint32_t takeNode(const Memory &memory, std::uint32_t index) noexcept;

// The five-way dispatch target for a node kind, from 0x80012F70..0x80012FA4. `kind` is the halfword at
// node+0x02. Anything at or above 5 falls to `kKindOther`, which is the common tail and not a
// handler, so this returns an ADDRESS and the caller can see that.
[[nodiscard]] constexpr std::uint32_t dispatchAddress(std::uint32_t kind) noexcept {
  if (kind == 0u) {
    return kKindZero;
  }
  if (kind == 1u) {
    return kKindOne;
  }
  if (kind == 2u) {
    return kKindTwo;
  }
  if (kind == 3u || kind == 4u) {
    return kKindThreeOrFour;
  }
  return kKindOther;
}

} // namespace crash1::pool_node
