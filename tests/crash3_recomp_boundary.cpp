#include "crash3_runtime.h"
#include "recomp_boundary_support.h"

constexpr std::string_view kCodeword = "SCUS-94244";

int main(int argc, char **argv) {
  return crash::test::TitleBoundaryRunner<crash3::Crash3Runtime>::run(argc, argv, kCodeword, gen_func_80048C38);
}
