#include "test_i3_support.h"

namespace prob_livo_test {
int RunP4InputSemanticsTests(TestContext &);
int RunP4MapTimingTests(TestContext &);
}  // namespace prob_livo_test

int main() {
  prob_livo_test::TestContext context;
  prob_livo_test::RunP4InputSemanticsTests(context);
  prob_livo_test::RunP4MapTimingTests(context);
  context.Print("G-P4 Super-input semantics and map-init timing gates");
  return context.Passed() ? 0 : 1;
}
