#include "test_i3_support.h"

namespace prob_livo_test {

int RunI3QrPlaneTests(TestContext &context) {
  LI2Sup::PlanePointsArray points;
  points[0] = LI2Sup::V3d(0.0, 0.0, 2.0);
  points[1] = LI2Sup::V3d(1.0, 0.0, 2.0);
  points[2] = LI2Sup::V3d(0.0, 1.0, 2.0);
  points[3] = LI2Sup::V3d(1.0, 1.0, 2.0);
  points[4] = LI2Sup::V3d(0.5, 0.3, 2.0);
  const LI2Sup::PlaneFitQr fit = LI2Sup::SolvePlaneFitQr(points, 5);
  context.Check(fit.solved && fit.legacy_accepted && fit.rank() == 3,
                "QR plane solve rejected a full-rank plane");
  const double scale = fit.q.norm();
  context.Check(std::abs(fit.q[0]) < 1e-12 && std::abs(fit.q[1]) < 1e-12 &&
                    std::abs(fit.q[2] + 0.5) < 1e-12,
                "QR plane coefficients do not match the production solve");
  context.Check(std::abs(1.0 / scale - 2.0) < 1e-12,
                "QR plane offset is incorrect");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
