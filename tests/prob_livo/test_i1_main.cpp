#include "test_support.h"

namespace prob_livo_test {
int RunLayoutTests(TestContext &context);
int RunRetractionTests(TestContext &context);
int RunPredictTests(TestContext &context);
int RunUpdateTests(TestContext &context);
int RunExposureTests(TestContext &context);
int RunSo3GoldenTests(TestContext &context);
}  // namespace prob_livo_test

int main() {
  prob_livo_test::TestContext layout;
  prob_livo_test::RunLayoutTests(layout);
  layout.Print("G-I1.1 layout/covariance embedding");

  prob_livo_test::TestContext retraction;
  prob_livo_test::RunRetractionTests(retraction);
  retraction.Print("G-I1.2 Super LIO retraction parity");

  prob_livo_test::TestContext predict;
  prob_livo_test::RunPredictTests(predict);
  predict.Print("G-I1.3/G-I1.4 nominal and covariance predict parity");

  prob_livo_test::TestContext update;
  prob_livo_test::RunUpdateTests(update);
  update.Print("G-I1.6 iterated LiDAR update parity");

  prob_livo_test::TestContext exposure;
  prob_livo_test::RunExposureTests(exposure);
  exposure.Print("G-I1.5/G-I1.7/G-I1.8 exposure and covariance validity");

  prob_livo_test::TestContext so3;
  prob_livo_test::RunSo3GoldenTests(so3);
  so3.Print("G-I1.SO3 actual Super golden parity");

  const bool passed = layout.Passed() && retraction.Passed() && predict.Passed() &&
                      update.Passed() && exposure.Passed() && so3.Passed();
  return passed ? 0 : 1;
}
