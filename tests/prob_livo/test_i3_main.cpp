#include "test_i3_support.h"

namespace prob_livo_test {
int RunI3MapInitTests(TestContext &);
int RunI3DownsampleTests(TestContext &);
int RunI3PointCovarianceTests(TestContext &);
int RunI3OctVoxTests(TestContext &);
int RunI3HknnTests(TestContext &);
int RunI3QrPlaneTests(TestContext &);
int RunI3QrCovarianceTests(TestContext &);
int RunI3AssociationWeightTests(TestContext &);
int RunI3BackendSeamTests(TestContext &);
}  // namespace prob_livo_test

int main() {
  prob_livo_test::TestContext context;
  prob_livo_test::RunI3MapInitTests(context);
  prob_livo_test::RunI3DownsampleTests(context);
  prob_livo_test::RunI3PointCovarianceTests(context);
  prob_livo_test::RunI3OctVoxTests(context);
  prob_livo_test::RunI3HknnTests(context);
  prob_livo_test::RunI3QrPlaneTests(context);
  prob_livo_test::RunI3QrCovarianceTests(context);
  prob_livo_test::RunI3AssociationWeightTests(context);
  prob_livo_test::RunI3BackendSeamTests(context);
  context.Print("G-I3 P0-P4 backend and native component gates");
  return context.Passed() ? 0 : 1;
}
