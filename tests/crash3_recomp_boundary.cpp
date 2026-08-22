#include "crash3_runtime.h"
#include "recomp_boundary_support.h"

namespace {

constexpr std::string_view kCodeword = "SCUS-94244";

void captureCall(Core *core) {
  crash::test::captureCallBoundary(core, kCodeword);
}

void captureEnterCritical(Core *core) {
  crash::test::executeEnterCritical(core, kCodeword, gen_func_80048C38);
}

} // namespace

int main(int argc, char **argv) {
  return crash::test::runBoundary<crash3::Crash3Runtime>(argc, argv, kCodeword, captureCall, captureEnterCritical);
}
