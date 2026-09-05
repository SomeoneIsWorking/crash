#include "crash1_boot_frontier.h"

#include <cstdio>

namespace {

int failures = 0;

#define CHECK(condition)                                                                                               \
  do {                                                                                                                 \
    if (!(condition)) {                                                                                                \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);                                        \
      ++failures;                                                                                                      \
    }                                                                                                                  \
  } while (false)

} // namespace

int main() {
  const crash1::BootFrontierFacts facts = crash1::bootFrontierFacts();
  CHECK(crash1::checkFirstBiosDispatch(facts.firstBiosDispatch, facts.firstBiosFunction, facts.firstBiosReturnAddress)
            .accepted);
  CHECK(!crash1::checkFirstBiosDispatch(
             facts.firstBiosDispatch ^ 4u, facts.firstBiosFunction, facts.firstBiosReturnAddress)
             .accepted);
  CHECK(!crash1::checkFirstBiosDispatch(
             facts.firstBiosDispatch, facts.firstBiosFunction ^ 1u, facts.firstBiosReturnAddress)
             .accepted);
  CHECK(!crash1::checkFirstBiosDispatch(
             facts.firstBiosDispatch, facts.firstBiosFunction, facts.firstBiosReturnAddress ^ 4u)
             .accepted);

  CHECK(crash1::checkPostGetC0Dispatch(facts.postGetC0Dispatch, facts.postGetC0Function, facts.postGetC0ReturnAddress)
            .accepted);
  CHECK(!crash1::checkPostGetC0Dispatch(
             facts.postGetC0Dispatch ^ 4u, facts.postGetC0Function, facts.postGetC0ReturnAddress)
             .accepted);
  CHECK(!crash1::checkPostGetC0Dispatch(
             facts.postGetC0Dispatch, facts.postGetC0Function ^ 1u, facts.postGetC0ReturnAddress)
             .accepted);
  CHECK(!crash1::checkPostGetC0Dispatch(
             facts.postGetC0Dispatch, facts.postGetC0Function, facts.postGetC0ReturnAddress ^ 4u)
             .accepted);

  if (failures == 0) {
    std::puts("Crash 1 boot frontier facts: 8/8 checks passed");
  }
  return failures == 0 ? 0 : 1;
}
