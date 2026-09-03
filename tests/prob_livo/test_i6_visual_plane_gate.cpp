#include "prob_livo/visual_plane_gate.h"
#include "test_support.h"

#include <Eigen/Geometry>

#include <cmath>
#include <limits>

namespace {

using prob_livo::EvaluateVisualPlaneGate;
using prob_livo::VisualPlaneGateInput;
using prob_livo::VisualPlaneGateMode;
using prob_livo_test::TestContext;

void TestProbabilisticGate(TestContext &context) {
  VisualPlaneGateInput input;
  input.geometry_valid = true;
  input.uncertainty_valid = true;
  input.normal = Eigen::Vector3d(0.0, 0.0, 1.0);
  input.radial_distance = 0.3;
  input.plane_radius = 0.1;
  input.plane_covariance = 0.01 * Eigen::Matrix4d::Identity();
  input.point_covariance = 0.04 * Eigen::Matrix3d::Identity();
  input.residual = 0.1;

  auto decision = EvaluateVisualPlaneGate(
      input, VisualPlaneGateMode::kLivo2Prob3sigma);
  context.Check(decision.radius_gate_pass, "A radius boundary must pass");
  context.Check(decision.uncertainty_valid && decision.second_gate_pass,
                "A accepted a valid 3sigma fixture");

  const double boundary = 3.0 * std::sqrt(decision.total_variance);
  input.residual = boundary;
  decision = EvaluateVisualPlaneGate(input,
                                     VisualPlaneGateMode::kLivo2Prob3sigma);
  context.Check(!decision.second_gate_pass,
                "A must keep the source strict < inequality");

  input.residual = boundary + 1e-6;
  decision = EvaluateVisualPlaneGate(input,
                                     VisualPlaneGateMode::kLivo2Prob3sigma);
  context.Check(!decision.second_gate_pass, "A rejected an outside residual");

  input.uncertainty_valid = false;
  decision = EvaluateVisualPlaneGate(input,
                                     VisualPlaneGateMode::kLivo2Prob3sigma);
  context.Check(!decision.second_gate_pass && !decision.uncertainty_valid,
                "A rejected missing uncertainty without a fallback");

  input.uncertainty_valid = true;
  input.plane_covariance(0, 0) = std::numeric_limits<double>::quiet_NaN();
  decision = EvaluateVisualPlaneGate(input,
                                     VisualPlaneGateMode::kLivo2Prob3sigma);
  context.Check(!decision.second_gate_pass,
                "A rejected non-finite plane covariance");
}

void TestLegacyGateAndSharedRadius(TestContext &context) {
  VisualPlaneGateInput input;
  input.geometry_valid = true;
  input.uncertainty_valid = false;
  input.normal = Eigen::Vector3d(0.0, 0.0, 1.0);
  input.radial_distance = 0.3;
  input.plane_radius = 0.1;
  input.sensor_range = 100.0;
  input.residual = 1.0;

  auto decision = EvaluateVisualPlaneGate(
      input, VisualPlaneGateMode::kSuperLegacy);
  context.Check(decision.radius_gate_pass && decision.second_gate_pass,
                "B must use the exact legacy range/error gate");

  input.residual = std::sqrt(input.sensor_range / 81.0);
  decision = EvaluateVisualPlaneGate(input, VisualPlaneGateMode::kSuperLegacy);
  context.Check(!decision.second_gate_pass,
                "B must keep the source strict > inequality");

  input.residual = 1.2;
  decision = EvaluateVisualPlaneGate(input, VisualPlaneGateMode::kSuperLegacy);
  context.Check(!decision.second_gate_pass, "B rejected an outside residual");

  input.radial_distance = 0.3000001;
  decision = EvaluateVisualPlaneGate(input, VisualPlaneGateMode::kSuperLegacy);
  context.Check(!decision.radius_gate_pass && !decision.second_gate_pass,
                "A/B must share the unchanged radius gate");

  input.radial_distance = 0.0;
  input.plane_covariance.setConstant(std::numeric_limits<double>::quiet_NaN());
  input.point_covariance.setConstant(std::numeric_limits<double>::quiet_NaN());
  input.residual = 0.5;
  decision = EvaluateVisualPlaneGate(input, VisualPlaneGateMode::kSuperLegacy);
  context.Check(decision.second_gate_pass,
                "B must not silently switch to covariance semantics");
}

void TestSensorRelativeRange(TestContext &context) {
  const Eigen::Matrix3d R_WI =
      Eigen::AngleAxisd(0.5, Eigen::Vector3d::UnitZ()).toRotationMatrix();
  const Eigen::Vector3d p_WI(10.0, -4.0, 2.0);
  const Eigen::Matrix3d R_IL =
      Eigen::AngleAxisd(-0.25, Eigen::Vector3d::UnitY()).toRotationMatrix();
  const Eigen::Vector3d t_IL(0.2, -0.1, 0.05);
  const Eigen::Vector3d point_W =
      R_WI * (R_IL * Eigen::Vector3d(3.0, 4.0, 12.0) + t_IL) + p_WI;

  const Eigen::Vector3d point_L = prob_livo::WorldToLidarPoint(
      point_W, R_WI, p_WI, R_IL, t_IL);
  context.Check((point_L - Eigen::Vector3d(3.0, 4.0, 12.0)).norm() < 1e-12,
                "world-to-LiDAR transform convention mismatch");
  context.Check(std::abs(prob_livo::WorldToLidarRange(
                          point_W, R_WI, p_WI, R_IL, t_IL) - 13.0) < 1e-12,
                "B range must be sensor-relative");
  context.Check(std::abs(point_W.norm() - 13.0) > 1.0,
                "fixture failed to distinguish world norm from sensor range");
}

}  // namespace

int main() {
  TestContext context;
  TestProbabilisticGate(context);
  TestLegacyGateAndSharedRadius(context);
  TestSensorRelativeRange(context);
  context.Print("G-I6 visual plane gate policy and sensor-range oracle");
  return context.Passed() ? 0 : 1;
}
