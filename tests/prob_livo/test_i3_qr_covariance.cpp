#include "test_i3_support.h"

namespace prob_livo_test {

int RunI3QrCovarianceTests(TestContext &context) {
  LI2Sup::PlanePointsArray points;
  points[0] = LI2Sup::V3d(0.0, 0.0, 2.0);
  points[1] = LI2Sup::V3d(1.0, 0.0, 2.0);
  points[2] = LI2Sup::V3d(0.0, 1.0, 2.0);
  points[3] = LI2Sup::V3d(1.0, 1.0, 2.0);
  points[4] = LI2Sup::V3d(0.5, 0.3, 2.0);
  LI2Sup::PlaneCovsArray covariances;
  for (auto &covariance : covariances)
    covariance = 0.001 * Eigen::Matrix3d::Identity();
  const LI2Sup::ProbQrPlane result =
      LI2Sup::ComputeProbQrPlane(points, covariances, 5);
  context.Check(result.status == LI2Sup::ProbQrPlane::kValid &&
                    result.covariance.allFinite(),
                "QR plane covariance was not valid for a full-rank plane");
  LI2Sup::PlanePointsArray degenerate;
  for (auto &point : degenerate) point = LI2Sup::V3d(1.0, 1.0, 1.0);
  const LI2Sup::ProbQrPlane invalid =
      LI2Sup::ComputeProbQrPlane(degenerate, covariances, 5);
  context.Check(invalid.status != LI2Sup::ProbQrPlane::kValid,
                "QR covariance accepted a rank-deficient neighborhood");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
