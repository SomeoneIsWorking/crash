#include "crash1_runtime.h"
#include "title_runtime_contract.h"

int main() {
  return verifyTitleRuntimeContract<crash1::Crash1Runtime>("Crash1Runtime", false);
}
