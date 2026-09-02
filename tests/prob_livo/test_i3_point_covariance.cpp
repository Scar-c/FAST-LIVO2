#include "test_i3_support.h"

namespace prob_livo_test {

int RunI3PointCovarianceTests(TestContext &context) {
  LI2Sup::VV3 points;
  points.push_back(LI2Sup::V3(1.2f, -0.4f, 5.0f));
  points.push_back(LI2Sup::V3(-0.8f, 0.7f, 4.0f));
  const Eigen::Matrix3d rotation =
      Eigen::AngleAxisd(0.35, Eigen::Vector3d::UnitZ()).toRotationMatrix();
  const Eigen::Vector3d translation(-0.05, 0.0, 0.055);
  std::vector<LI2Sup::M3d> covariances;
  LI2Sup::ComputeBodyCovListWithExtrinsic(points, rotation, translation,
                                          0.02, 0.01, covariances);
  context.Check(covariances.size() == points.size(),
                "body covariance list has stale or missing entries");
  for (const auto &covariance : covariances) {
    context.Check(LI2Sup::ValidateCovariance(
                      covariance, LI2Sup::CovValidationMode::Full, 1e-9),
                  "body covariance is not finite symmetric PSD");
  }
  const Eigen::Vector3d recovered =
      rotation * (rotation.transpose() * (points[0].cast<double>() - translation)) +
      translation;
  context.Check((recovered - points[0].cast<double>()).norm() < 1e-12,
                "lidar-to-IMU extrinsic round trip changed the point frame");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
