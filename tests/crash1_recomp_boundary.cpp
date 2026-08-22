#include "crash1_runtime.h"
#include "recomp_boundary_support.h"

namespace {

constexpr std::string_view kCodeword = "SCUS-94900";

void captureCall(Core *core) {
  crash::test::captureCallBoundary(core, kCodeword);
}

void captureEnterCritical(Core *core) {
  crash::test::executeEnterCritical(core, kCodeword, gen_func_8003E1F8);
}

} // namespace

int main(int argc, char **argv) {
  return crash::test::runBoundary<crash1::Crash1Runtime>(argc, argv, kCodeword, captureCall, captureEnterCritical);
}
