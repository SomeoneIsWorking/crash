#include "crash1_boot_frontier.h"

#include "crash1_runtime.h"

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

void transition(Core *) {}
void boundary(Core *) {}

} // namespace

int main() {
  crash1::Crash1Runtime runtime;
  const crash1::BootFrontierFacts facts = crash1::bootFrontierFacts();
  const crash::ResidentProgram program = crash1::makeBootFrontierProgram(runtime, "retail.exe", transition, boundary);

  CHECK(program.entry == runtime.guestProgramImage()->crt0Entry);
  CHECK(program.boundary == facts.postGetC0Dispatch);
  CHECK(program.boundaryHandler == boundary);
  CHECK(program.boundaryKind == crash::ResidentBoundaryKind::DynamicDispatch);
  CHECK(program.transitionBoundary == facts.firstSyscall);
  CHECK(program.transitionHandler == transition);

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
    std::puts("Crash 1 boot frontier: 14/14 checks passed");
  }
  return failures == 0 ? 0 : 1;
}
