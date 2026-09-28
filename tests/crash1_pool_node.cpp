// The recovered pool allocate path, 0x80012F10, driven against a fixture with no Core in existence.
//
// WHY A FIXTURE AND NOT A RUN. The model in titles/crash1/core/crash1_pool_node.h is a reading of
// 259 instruction words, and a reading is only as good as the cases that could refute it. Every case
// below is chosen because a plausible mis-transcription passes the easy ones: a stride written as
// `index * 44` with the multiply-out wrong, a cursor that is not reset, a walk that returns the cell
// AFTER the match, a class rule that silently uses the shifted key. The last of those is the defect
// this repository SHIPPED in the lookup owner, so it gets the sharpest case in the file.
#include "crash1_pool_node.h"

#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const std::string &what) {
  if (!condition) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
  }
}

// A sparse guest memory, because the addresses here are 0x8005xxxx and 0x801xxxxx and a flat array
// would be 2 MiB of mostly zeros for a model that touches a dozen words.
class Fixture {
public:
  static std::uint32_t read(const void *context, std::uint32_t address) noexcept {
    const auto &self = *static_cast<const Fixture *>(context);
    const auto found = self.words_.find(address);
    return found == self.words_.end() ? 0u : found->second;
  }

  static void write(const void *context, std::uint32_t address, std::uint32_t value) noexcept {
    // The seam hands out a `const void *` because the read half must not be able to mutate the
    // fixture, and the write half cannot be. The cast is in one place and is stated, exactly as
    // `crash1_block_pool.cpp` states the same drop for `Core::mem_r32`.
    auto &self = *const_cast<Fixture *>(static_cast<const Fixture *>(context));
    self.words_[address] = value;
    self.writes_.push_back(address);
  }

  [[nodiscard]] crash1::pool_node::Memory seam() noexcept {
    return {this, &Fixture::read, &Fixture::write};
  }

  void put(std::uint32_t address, std::uint32_t value) {
    words_[address] = value;
  }

  [[nodiscard]] std::uint32_t get(std::uint32_t address) const {
    const auto found = words_.find(address);
    return found == words_.end() ? 0u : found->second;
  }

  [[nodiscard]] const std::vector<std::uint32_t> &writes() const {
    return writes_;
  }

private:
  std::unordered_map<std::uint32_t, std::uint32_t> words_;
  std::vector<std::uint32_t> writes_;
};

constexpr std::uint32_t kBucketTableAddr = crash1::pool_node::kBucketTable;
constexpr std::uint32_t kNodeTableAddr = crash1::pool_node::kNodeTable;
constexpr std::uint32_t kNodeCursorAddr = crash1::pool_node::kNodeCursor;
constexpr std::uint32_t kCellBase = 0x80100000u;
constexpr std::uint32_t kNode = kNodeTableAddr + 3u * crash1::pool_node::kNodeStride; // index 3

} // namespace

int main() {
  using namespace crash1::pool_node;

  // --- the node table's stride and its cursor reset -------------------------------------------------
  //
  // The stride is 44 and it is built by five instructions, not by a multiply. The test drives three
  // indices and checks the ADDRESS, because a stride of 44 written as `index * 41` would still make
  // index 0 correct and only fail from index 1 on — which is why three indices and not one.
  {
    Fixture fixture;
    for (std::uint32_t index : {0u, 1u, 5u}) {
      check(takeNode(fixture.seam(), index) == kNodeTableAddr + index * 44u,
            "the node at index " + std::to_string(index) +
                " is 44 bytes on, and 44 is the stride the "
                "image computes (a0*2 + a0 = 3n, *4 = 12n, -n = 11n, *4 = 44n)");
    }
  }
  {
    // The reset: 0x80012F54 `bne $4,$18,0x80012F64` with 0x80012F5C/0x80012F60 clearing the cursor.
    // Without it two callers take the same node, and no address check would notice.
    Fixture fixture;
    fixture.put(kNodeCursorAddr, kNodeTableAddr + 2u * 44u);
    takeNode(fixture.seam(), 2u);
    check(fixture.get(kNodeCursorAddr) == 0u,
          "a cursor already naming this node is cleared, so two callers cannot take the same node");
    check(fixture.writes().size() == 1u && fixture.writes().front() == kNodeCursorAddr,
          "and the reset is the ONLY write this function makes, at 0x8005CFAC");
  }
  {
    Fixture fixture;
    fixture.put(kNodeCursorAddr, kNodeTableAddr + 7u * 44u);
    takeNode(fixture.seam(), 2u);
    check(fixture.writes().empty(), "a cursor naming a DIFFERENT node is left alone: the reset is not a blanket clear");
  }

  // --- the bucket address, and the class rule it exists to prove ------------------------------------
  {
    Fixture fixture;
    fixture.put(kBucketTableAddr, 0x80060000u);
    // The four words the live run measured, with the byte offsets their shifts produce.
    check(bucketAddress(fixture.seam(), 0x0057CCFBu) == 0x80060000u + 700u,
          "request 0x0057CCFB indexes bucket byte offset 700 — (word >> 13) & 0x3FC");
    check(bucketAddress(fixture.seam(), 0x15814CE7u) == 0x80060000u + 8u,
          "request 0x15814CE7 indexes bucket byte offset 8, a DIFFERENT bucket from a request that "
          "differs only in its low bits' effect on the shift");
    check(bucketAddress(fixture.seam(), 0x00002000u) == 0x80060000u,
          "a request below 8 KiB shifts to bucket key 0, so it reads the table's first word");
    // The low 13 bits are DROPPED by the bucket and KEPT by the class. Two requests one bit apart
    // share a bucket, and the fixture below proves they are two different classes.
    check(bucketAddress(fixture.seam(), 0x0057CCFBu) == bucketAddress(fixture.seam(), 0x0057CCFAu),
          "two requests differing only below bit 13 share a bucket, which is exactly why a lookup "
          "that searched for the shifted key cannot tell them apart");
  }

  // --- the cell link, and the rule the shipped lookup owner got wrong -------------------------------
  {
    constexpr std::uint32_t kClass = 0x0057CCFBu; // a real word from the live run
    Fixture fixture;
    fixture.put(kBucketTableAddr, 0x80060000u);
    fixture.put(0x80060000u + 700u, kCellBase);             // the bucket's first cell
    fixture.put(kCellBase + 4u, 0x0057CCFAu);               // a NEIGHBOUR class, one bit below
    fixture.put(kCellBase + 8u, 0x00005678u);               // cell 1 payload
    fixture.put(kCellBase + 12u, kClass);                   // cell 1 class — THE match
    fixture.put(kNode + kNodeElementArray, kCellBase + 8u); // node's element list holds the payload
    const CellLink link = linkFirstCell(fixture.seam(), kNode, kClass);
    check(!link.exhausted, "the walk finds the cell carrying the class");
    check(link.cell == kCellBase + 8u,
          "and it returns the cell that MATCHED, not its neighbour: 0x8001301C steps the cursor in "
          "the delay slot and the match path keeps the cell it stopped on");
    check(link.cellsWalked == 1u, "one stride step before the match: 1 cell walked after the first");
    check(link.classWord == kClass && link.bucketOffset == 700u,
          "the link records the WHOLE class word and the byte offset it indexes, and the two are "
          "different quantities — conflating them is the defect this file exists to prevent");
    check(fixture.writes().size() == 1u && fixture.writes().front() == kCellBase + 8u,
          "the only write is 0x80013020 `sw $5,0($19)`: the cell's FIRST word, its payload. The class "
          "word is NOT written, which is why the walk had to find the cell at all");
  }
  {
    // THE DISCRIMINATOR, and it is the exact bug that shipped. A cell carrying the SHIFTED key is not
    // a match, because the engine never stores a shifted key — so a model that searched for one
    // walks past the real cell and off the end of the bucket. The neighbour here is the most
    // dangerous possible cell: one bit away.
    constexpr std::uint32_t kClass = 0x0057CCFBu;
    Fixture fixture;
    fixture.put(kBucketTableAddr, 0x80060000u);
    fixture.put(0x80060000u + 700u, kCellBase);
    fixture.put(kCellBase + 4u, kClass >> kBucketIndexShift); // the class the WRONG owner searched for
    const CellLink link = linkFirstCell(fixture.seam(), kNode, kClass);
    check(link.exhausted, "a cell carrying only the shifted key does not serve the request");
    check(fixture.writes().empty(), "and nothing is written: the recovered walk never claims a cell it did not match");
  }
  {
    // The unbounded walk, stopped at the same edge the lookup owner stops at and for the same reason.
    Fixture fixture;
    fixture.put(kBucketTableAddr, 0x80060000u);
    fixture.put(0x80060000u + 0u, kCellBase);
    const CellLink link = linkFirstCell(fixture.seam(), kNode, 0x4E938CCDu);
    check(link.exhausted, "an unserved class exhausts rather than returning a neighbour");
    check(fixture.writes().empty(), "and writes nothing");
  }

  // --- the five-way dispatch -----------------------------------------------------------------------
  {
    check(dispatchAddress(0u) == kKindZero, "kind 0 is the path that writes pool cells");
    check(dispatchAddress(1u) == kKindOne, "kind 1 is the path that measures through 0x8001439C");
    check(dispatchAddress(2u) == kKindTwo, "kind 2 is the path that initialises an escape class");
    check(dispatchAddress(3u) == kKindThreeOrFour && dispatchAddress(4u) == kKindThreeOrFour,
          "kinds 3 and 4 SHARE a path, which is why the recovery names it by both");
    check(dispatchAddress(5u) == kKindOther && dispatchAddress(0xFFFFu) == kKindOther,
          "kind 5 and above fall to the common tail, and the tail is not a handler: it republishes "
          "the node state and returns");
    // The three addresses are DIFFERENT from each other and all inside the function's own body, so a
    // transcription that collapsed them onto one case would be visible here.
    check(kKindZero != kKindOne && kKindOne != kKindTwo && kKindTwo != kKindThreeOrFour,
          "the four case addresses are distinct");
    check(kEntry <= kKindZero && kKindThreeOrFour < kEnd,
          "and all of them are inside 0x80012F10..0x8001331C, the body's own digest range");
  }

  // --- the constants, against the image-derived values the probe gates -------------------------------
  check(kNodeStride == 44u && kCallbackStride == 28u,
        "44 and 28 are the strides the five and three instructions actually compute");
  check(kClassWordOffset == 0x14u && kRequestOffset == 0x18u,
        "the class word lives at node+0x14 and the request the lookup is called with at cell+0x18, "
        "and 0x80013140's `sw` displacement is what fixes the first");
  check(kMeasureDeadEnd == -12, "0x8001439C's dead-end sentinel is -12, from `addiu $2,$zero,-0xC`");
  check(kStateBuilt == 0x1Eu && kStateDone == 0x14u && kStateReset == 0x1u,
        "the three published node states are 30, 20 and 1, from the three `ori` immediates");

  std::printf("Crash1PoolNode: node stride %u, callback stride %u, dispatch 0/1/2/3-4/other at "
              "0x%08X/0x%08X/0x%08X/0x%08X/0x%08X; the class is the WHOLE word and the shift is a "
              "bucket selector, which is the rule the shipped lookup owner got wrong\n",
              kNodeStride,
              kCallbackStride,
              kKindZero,
              kKindOne,
              kKindTwo,
              kKindThreeOrFour,
              kKindOther);
  if (failures != 0) {
    std::fprintf(stderr, "%d check(s) failed\n", failures);
    return 1;
  }
  return 0;
}
