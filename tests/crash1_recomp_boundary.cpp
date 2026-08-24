#include "crash1_runtime.h"
#include "recomp_boundary_support.h"

constexpr std::string_view kCodeword = "SCUS-94900";

int main(int argc, char **argv) {
  return crash::test::TitleBoundaryRunner<crash1::Crash1Runtime>::run(argc, argv, kCodeword, gen_func_8003E1F8);
}
