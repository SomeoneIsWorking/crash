#include "crash1_horizontal_bound.h"

#include "core.h"
#include "crash1_runtime.h"
#include "dynarec_dispatch.h"
#include "game.h"

#include <cstdlib>
#include <lucent/log.h>

namespace crash1 {
namespace {

[[noreturn]] void refuse(const char *what, std::int32_t observed) {
  lucent::error("crash1-bound",
                "SCUS-949.00 horizontal bound {}; the guest's own camera setup cannot publish it, so "
                "something moved the bound a widening must hold fixed",
                what,
                observed);
  std::abort();
}

void originalSubmitter(Core &core) {
  crash::dynarec::requireGuestReturn(crash::dynarec::callOriginal(core, kHorizontalSubmitter),
                                     "crash1-bound::submitter original");
}

void submitterOverride(Core *core) {
  Crash1HorizontalBound::from(*core).observeSubmitter(*core, originalSubmitter);
}

} // namespace

void Crash1HorizontalBound::observeSubmitter(Core &core, const RetailBody &retail) {
  if (!retail) {
    refuse("observation reached without the retail submitter body", 0);
  }
  // PRE-GTE game state, read not written: this is the main-RAM global the camera setup's single
  // store published, and it is the exact value FUN_8001DE78 is about to consume at 0x8001DF6C /
  // 0x8001DFFC / 0x8001E048. Reading it here is a reading of the title's own projection state, not
  // an observation of what the GTE produced — the GTE is not involved at this point, and nothing in
  // this owner writes guest memory.
  const auto consumed = static_cast<std::int32_t>(core.mem_r32(kHorizontalBound));
  if (!isRetailHorizontalBound(consumed)) {
    refuse("is not a value the measured camera setup publishes", consumed);
  }

  retail(core);

  // The retail submitter does not write the bound; if it did, the value recorded here would be a
  // value this owner chose rather than one the title's next object will read, so the difference is
  // named instead of being silently re-read.
  const auto after = static_cast<std::int32_t>(core.mem_r32(kHorizontalBound));
  if (!Crash1HorizontalBound::boundIntact(consumed, after)) {
    refuse("was moved by the submitter it is supposed to read", after);
  }

  // First-seen order, and a value already seen is not appended twice, so the set stays a count of
  // DISTINCT bounds. `submissions()` is the denominator a run must report next to it: a set of one
  // with many submissions says the title held its bound, and a set of one with one submission says
  // almost nothing.
  bool already = false;
  for (const std::int32_t seen : observedBounds_) {
    already = already || seen == consumed;
  }
  if (!already) {
    observedBounds_.push_back(consumed);
  }
  lastObservedBound_ = consumed;
  ++submissions_;
}

Crash1HorizontalBound &Crash1HorizontalBound::from(Core &core) {
  if (!core.runtime) {
    lucent::error("crash1-bound", "SCUS-949.00 horizontal-bound override ran without its title runtime");
    std::abort();
  }
  // The owner is per-title state behind the runtime, so the checked downcast is the whole lookup: a
  // Core running another title's runtime is a wiring defect, and saying so beats reading a
  // neighbour's bound and calling it this title's.
  // The const is dropped on purpose and only here: the owner holds per-title counters that this
  // override increments, and it is reached from a non-const Core. The downcast still has to succeed,
  // so a Core running another title is still a named refusal rather than a neighbour's counters.
  auto *const runtime = dynamic_cast<Crash1Runtime *>(const_cast<GameRuntime *>(core.runtime));
  if (!runtime) {
    lucent::error("crash1-bound", "SCUS-949.00 horizontal-bound override reached another title's runtime");
    std::abort();
  }
  return runtime->horizontalBound();
}

void installCrash1HorizontalBound(Core &core) {
  if (!crash::dynarec::installOverride(
          core, kHorizontalSubmitter, "Crash GoolObjectTransform bound", submitterOverride)) {
    std::abort();
  }
  lucent::info("crash1-bound",
               "horizontal-bound owner installed: submitter 0x{:08X} over the bound at 0x{:08X} "
               "(written only at 0x{:08X}, re-sent to CR[26] at 0x{:08X}); near-plane consumer "
               "0x{:08X} far limit {}",
               kHorizontalSubmitter,
               kHorizontalBound,
               kHorizontalBoundWriter,
               kHorizontalBoundResend,
               kHorizontalNearPlane,
               kHorizontalFarLimit);
}

} // namespace crash1
