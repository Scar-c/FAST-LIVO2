#include "test_i3_support.h"

namespace prob_livo_test {

int RunI3AssociationWeightTests(TestContext &context) {
  const LI2Sup::ProbWeight fixed =
      LI2Sup::ComputeP2pProbWeight(0.0, 0.0, 0.001);
  context.Check(fixed.valid && std::abs(fixed.weight - 1000.0) < 1e-12,
                "P4 zero-variance weight is not the fixed-1000 limit");
  const LI2Sup::ProbWeight weighted =
      LI2Sup::ComputeP2pProbWeight(0.2, 0.3, 0.001);
  context.Check(weighted.valid &&
                    std::abs(weighted.weight - 1.0 / 0.501) < 1e-12,
                "P4 probabilistic weight formula changed");
  const LI2Sup::ProbWeight invalid =
      LI2Sup::ComputeP2pProbWeight(-0.1, 0.0, 0.001);
  context.Check(!invalid.valid && invalid.invalid_negative,
                "P4 materially negative variance was not rejected");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
