#include "crash2_runtime.h"
#include "recomp_boundary_support.h"

constexpr std::string_view kCodeword = "SCUS-94154";

int main(int argc, char **argv) {
  return crash::test::TitleBoundaryRunner<crash2::Crash2Runtime>::run(argc, argv, kCodeword, gen_func_80049D1C);
}
