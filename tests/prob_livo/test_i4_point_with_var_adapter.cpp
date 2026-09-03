#include "prob_livo/prob_point_with_var_adapter.h"
#include "test_i3_support.h"
#include "vio.h"

#include <Eigen/Geometry>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace {

using prob_livo::Layout;
using prob_livo::Matrix19;
using prob_livo::ProbPointIdentity;
using prob_livo::ProbPointWithVarAdapter;
using prob_livo::ProbPointWithVarBuffer;
using prob_livo_test::TestContext;

Eigen::Matrix3d Skew(const Eigen::Vector3d &value) {
  Eigen::Matrix3d result;
  result << 0.0, -value.z(), value.y(), value.z(), 0.0, -value.x(),
      -value.y(), value.x(), 0.0;
  return result;
}

PointCloudXYZI MakeScan() {
  PointCloudXYZI scan;
  PointType first;
  first.x = 1.0f;
  first.y = 2.0f;
  first.z = 3.0f;
  first.intensity = 12.5f;
  first.curvature = 4.25f;
  scan.push_back(first);

  PointType second;
  second.x = -2.0f;
  second.y = 0.5f;
  second.z = 1.25f;
  second.intensity = 7.0f;
  second.curvature = 8.5f;
  scan.push_back(second);
  return scan;
}

LI2Sup::VV3 MakeImuPoints() {
  LI2Sup::VV3 points;
  points.emplace_back(1.0f, 2.0f, 3.0f);
  points.emplace_back(-2.0f, 0.5f, 1.25f);
  return points;
}

std::vector<LI2Sup::M3d> MakeLidarCovariances() {
  std::vector<LI2Sup::M3d> covariances(2, LI2Sup::M3d::Zero());
  covariances[0] << 0.04, 0.01, -0.003, 0.01, 0.09, 0.006, -0.003,
      0.006, 0.16;
  covariances[1] << 0.02, -0.004, 0.002, -0.004, 0.07, 0.005, 0.002,
      0.005, 0.11;
  return covariances;
}

Matrix19 MakeStateCovariance() {
  Matrix19 covariance = Matrix19::Zero();
  covariance.block<3, 3>(Layout::kRot0, Layout::kRot0)
      << 0.11, 0.013, -0.007, 0.013, 0.17, 0.009, -0.007, 0.009, 0.23;
  covariance.block<3, 3>(Layout::kPos0, Layout::kPos0)
      << 0.31, -0.021, 0.014, -0.021, 0.27, 0.011, 0.014, 0.011, 0.19;
  covariance.block<3, 3>(Layout::kRot0, Layout::kPos0)
      << 0.004, -0.002, 0.001, 0.003, 0.005, -0.004, -0.001, 0.002,
      0.006;
  covariance.block<3, 3>(Layout::kPos0, Layout::kRot0) =
      covariance.block<3, 3>(Layout::kRot0, Layout::kPos0).transpose();
  covariance(Layout::kExpo, Layout::kExpo) = 17.0;
  covariance(Layout::kExpo, Layout::kRot0) = 2.0;
  covariance(Layout::kRot0, Layout::kExpo) = 2.0;
  return covariance;
}

std::vector<std::uint8_t> MakeAcceptedMask() { return {1, 0}; }

std::vector<Eigen::Vector4d> MakePlaneCoefficients() {
  std::vector<Eigen::Vector4d> coefficients(2, Eigen::Vector4d::Zero());
  coefficients[0] << 0.2, -0.3, 0.932737905, -1.7;
  return coefficients;
}

void CheckClose(TestContext &context, double error, double tolerance,
                const std::string &name) {
  context.Record(name, error);
  context.Check(error <= tolerance, name + " mismatch");
}

bool BuildAdapter(ProbPointWithVarAdapter &adapter, std::string &error,
                  const PointCloudXYZI &scan, const LI2Sup::VV3 &points,
                  const std::vector<LI2Sup::M3d> &lidar_covariances,
                  const std::vector<LI2Sup::M3d> &imu_covariances,
                  const Eigen::Matrix3d &world_rotation,
                  const Eigen::Vector3d &world_translation,
                  const Matrix19 &state_covariance,
                  const Eigen::Matrix3d &lidar_to_imu_rotation,
                  const Eigen::Vector3d &lidar_to_imu_translation,
                  const std::vector<std::uint8_t> &accepted_mask,
                  const std::vector<Eigen::Vector4d> &plane_coefficients) {
  return adapter.Build(42, scan, points, lidar_covariances, imu_covariances,
                       world_rotation, world_translation, state_covariance,
                       lidar_to_imu_rotation, lidar_to_imu_translation,
                       accepted_mask, plane_coefficients, error);
}

void TestIdentityAndOrdering(TestContext &context) {
  const PointCloudXYZI scan = MakeScan();
  const LI2Sup::VV3 points = MakeImuPoints();
  const std::vector<LI2Sup::M3d> lidar_covariances = MakeLidarCovariances();
  const std::vector<LI2Sup::M3d> imu_covariances = lidar_covariances;
  const Matrix19 state_covariance = Matrix19::Zero();
  ProbPointWithVarAdapter adapter;
  std::string error;
  const bool ok = BuildAdapter(
      adapter, error, scan, points, lidar_covariances, imu_covariances,
      Eigen::Matrix3d::Identity(), Eigen::Vector3d::Zero(), state_covariance,
      Eigen::Matrix3d::Identity(), Eigen::Vector3d::Zero(),
      MakeAcceptedMask(), MakePlaneCoefficients());
  context.Check(ok, "identity adapter build failed: " + error);
  if (!ok) return;

  const ProbPointWithVarBuffer &output = adapter.output();
  context.Check(output.points.size() == 2, "identity output count mismatch");
  context.Check(output.identities.size() == 2, "identity metadata count mismatch");
  context.Check(output.normal_valid.size() == 2,
                "identity normal validity count mismatch");
  context.Check(output.normals_available,
                "accepted normal capability must be available");
  for (std::size_t index = 0; index < output.points.size(); ++index) {
    const pointWithVar &point = output.points[index];
    const ProbPointIdentity &identity = output.identities[index];
    CheckClose(context, (point.point_i - points[index].cast<double>()).norm(),
               0.0, "identity.point_i");
    CheckClose(context, (point.point_b - points[index].cast<double>()).norm(),
               0.0, "identity.point_b");
    CheckClose(context, (point.point_w - points[index].cast<double>()).norm(),
               0.0, "identity.point_w");
    CheckClose(context,
               (point.body_var - lidar_covariances[index])
                   .cwiseAbs()
                   .maxCoeff(),
               0.0, "identity.body_var_lidar_frame");
    CheckClose(context,
               (point.var - imu_covariances[index]).cwiseAbs().maxCoeff(), 0.0,
               "identity.world_var");
    CheckClose(context,
               (point.var_nostate - imu_covariances[index])
                   .cwiseAbs()
                   .maxCoeff(),
               0.0, "identity.var_nostate");
    Eigen::Vector3d expected_normal = Eigen::Vector3d::Zero();
    if (index == 0) expected_normal = MakePlaneCoefficients()[0].head<3>();
    CheckClose(context, (point.normal - expected_normal).norm(), 0.0,
               "identity.normal_source");
    context.Check(output.normal_valid[index] == (index == 0),
                  "identity normal validity mismatch");
    context.Check(identity.scan_id == 42, "identity scan id mismatch");
    context.Check(identity.source_index == index, "identity source order mismatch");
    context.Check(identity.intensity == scan.points[index].intensity,
                  "identity intensity mismatch");
    context.Check(identity.relative_time_ms == scan.points[index].curvature,
                  "identity relative time mismatch");
  }
}

void TestCoordinateAndCovarianceFrames(TestContext &context) {
  const PointCloudXYZI scan = MakeScan();
  const LI2Sup::VV3 points = MakeImuPoints();
  const std::vector<LI2Sup::M3d> lidar_covariances = MakeLidarCovariances();
  const Eigen::Matrix3d world_rotation =
      Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d::UnitZ())
          .toRotationMatrix();
  const Eigen::Vector3d world_translation(10.0, -2.0, 0.5);
  const Eigen::Matrix3d lidar_to_imu_rotation =
      Eigen::AngleAxisd(0.3, Eigen::Vector3d::UnitY()).toRotationMatrix();
  const Eigen::Vector3d lidar_to_imu_translation(0.2, -0.1, 0.3);
  std::vector<LI2Sup::M3d> imu_covariances(2);
  for (std::size_t index = 0; index < imu_covariances.size(); ++index) {
    imu_covariances[index] = lidar_to_imu_rotation * lidar_covariances[index] *
                             lidar_to_imu_rotation.transpose();
  }
  const Matrix19 state_covariance = MakeStateCovariance();

  ProbPointWithVarAdapter adapter;
  std::string error;
  const bool ok = BuildAdapter(
      adapter, error, scan, points, lidar_covariances, imu_covariances,
      world_rotation, world_translation, state_covariance,
      lidar_to_imu_rotation, lidar_to_imu_translation, {1, 1},
      {Eigen::Vector4d(0.2, -0.3, 0.932737905, -1.7),
       Eigen::Vector4d(-0.4, 0.1, 0.9, 2.3)});
  context.Check(ok, "frame adapter build failed: " + error);
  if (!ok) return;

  const pointWithVar &first = adapter.output().points.front();
  const Eigen::Vector3d point_imu(1.0, 2.0, 3.0);
  const Eigen::Vector3d expected_world =
      world_rotation * point_imu + world_translation;
  const Eigen::Vector3d expected_lidar =
      lidar_to_imu_rotation.transpose() *
      (point_imu - lidar_to_imu_translation);
  const Eigen::Matrix3d expected_world_sensor =
      world_rotation * imu_covariances.front() * world_rotation.transpose();
  const Eigen::Matrix3d expected_world_full =
      expected_world_sensor +
      (-Skew(point_imu)) *
          state_covariance.block<3, 3>(Layout::kRot0, Layout::kRot0) *
          (-Skew(point_imu)).transpose() +
      state_covariance.block<3, 3>(Layout::kPos0, Layout::kPos0);
  CheckClose(context, (first.point_i - point_imu).norm(), 1e-12,
             "frames.point_i");
  CheckClose(context, (first.point_b - expected_lidar).norm(), 1e-12,
             "frames.inverse_extrinsic");
  CheckClose(context, (first.point_w - expected_world).norm(), 1e-12,
             "frames.world_point");
  CheckClose(context,
             (first.point_crossmat - Skew(point_imu)).cwiseAbs().maxCoeff(),
             1e-12, "frames.point_crossmat");
  CheckClose(context,
             (first.body_var - lidar_covariances.front())
                 .cwiseAbs()
                 .maxCoeff(),
             1e-12, "frames.body_var_lidar_covariance");
  CheckClose(context,
             (first.var - expected_world_full).cwiseAbs().maxCoeff(), 1e-12,
             "frames.world_full_covariance");
  CheckClose(context,
             (first.var_nostate - expected_world_sensor)
                 .cwiseAbs()
                 .maxCoeff(),
             1e-12, "frames.world_sensor_covariance");
  const double state_covariance_delta =
      (first.var - first.var_nostate).cwiseAbs().maxCoeff();
  context.Record("frames.state_covariance_is_present", state_covariance_delta);
  context.Check(state_covariance_delta > 1e-4,
                "frames.state covariance term is missing");

  const Eigen::Matrix3d wrong_body_frame = imu_covariances.front();
  const Eigen::Matrix3d wrong_sensor_rotation =
      world_rotation * lidar_covariances.front() * world_rotation.transpose();
  const Eigen::Matrix3d double_rotated =
      world_rotation * expected_world_sensor * world_rotation.transpose();
  const Eigen::Matrix3d no_state_covariance = expected_world_sensor;
  context.Check((first.body_var - wrong_body_frame).cwiseAbs().maxCoeff() > 1e-4,
                "negative body_var=Sigma_I was not detected");
  context.Check((first.var_nostate - wrong_sensor_rotation)
                    .cwiseAbs()
                    .maxCoeff() > 1e-4,
                "negative wrong sensor rotation was not detected");
  context.Check((first.var_nostate - double_rotated).cwiseAbs().maxCoeff() >
                    1e-4,
                "negative double covariance rotation was not detected");
  context.Check((first.var - no_state_covariance).cwiseAbs().maxCoeff() >
                    1e-4,
                "negative var=var_nostate was not detected");
  context.Check((first.point_w -
                 (world_rotation * (lidar_to_imu_rotation * point_imu +
                                    lidar_to_imu_translation) +
                  world_translation))
                    .norm() > 1e-4,
                "negative double extrinsic application was not detected");
  context.Check((first.point_w -
                 (world_rotation.transpose() * point_imu + world_translation))
                    .norm() > 1e-4,
                "negative wrong pose rotation was not detected");
}

void TestInputIsolation(TestContext &context) {
  const PointCloudXYZI scan = MakeScan();
  const PointCloudXYZI scan_before = scan;
  const LI2Sup::VV3 points = MakeImuPoints();
  const LI2Sup::VV3 points_before = points;
  const std::vector<LI2Sup::M3d> lidar_covariances = MakeLidarCovariances();
  const std::vector<LI2Sup::M3d> lidar_covariances_before = lidar_covariances;
  const std::vector<LI2Sup::M3d> imu_covariances = lidar_covariances;
  const std::vector<std::uint8_t> accepted_mask = MakeAcceptedMask();
  const std::vector<Eigen::Vector4d> plane_coefficients =
      MakePlaneCoefficients();

  ProbPointWithVarAdapter adapter;
  std::string error;
  const bool ok = BuildAdapter(
      adapter, error, scan, points, lidar_covariances, imu_covariances,
      Eigen::Matrix3d::Identity(), Eigen::Vector3d::Zero(), Matrix19::Zero(),
      Eigen::Matrix3d::Identity(), Eigen::Vector3d::Zero(), accepted_mask,
      plane_coefficients);
  context.Check(ok, "input-isolation adapter build failed: " + error);
  if (!ok) return;

  for (std::size_t index = 0; index < scan.size(); ++index) {
    context.Check(scan.points[index].x == scan_before.points[index].x &&
                      scan.points[index].y == scan_before.points[index].y &&
                      scan.points[index].z == scan_before.points[index].z &&
                      scan.points[index].intensity ==
                          scan_before.points[index].intensity &&
                      scan.points[index].curvature ==
                          scan_before.points[index].curvature,
                  "adapter mutated authoritative scan");
    CheckClose(context, (points[index] - points_before[index]).norm(), 0.0,
               "input-isolation.points");
    CheckClose(context,
               (lidar_covariances[index] - lidar_covariances_before[index])
                   .cwiseAbs()
                   .maxCoeff(),
               0.0, "input-isolation.lidar_covariance");
  }
}

void TestRealFastConsumerSemantics(TestContext &context) {
  const PointCloudXYZI scan = MakeScan();
  const LI2Sup::VV3 points = MakeImuPoints();
  const std::vector<LI2Sup::M3d> lidar_covariances = MakeLidarCovariances();
  ProbPointWithVarAdapter adapter;
  std::string error;
  const bool ok = BuildAdapter(
      adapter, error, scan, points, lidar_covariances, lidar_covariances,
      Eigen::Matrix3d::Identity(), Eigen::Vector3d::Zero(), Matrix19::Zero(),
      Eigen::Matrix3d::Identity(), Eigen::Vector3d::Zero(), {1, 0},
      {Eigen::Vector4d(0.0, 0.0, 1.0, -3.0), Eigen::Vector4d::Zero()});
  context.Check(ok, "consumer seam adapter build failed: " + error);
  if (!ok) return;

  pointWithVar accepted_candidate;
  pointWithVar rejected_candidate;
  const auto &points_with_var = adapter.output().points;
  const bool accepted_reaches =
      PrepareVisualMapCandidate(points_with_var[0], accepted_candidate);
  const bool rejected_skips =
      !PrepareVisualMapCandidate(points_with_var[1], rejected_candidate);
  context.Check(accepted_reaches,
                "accepted adapted point was skipped by FAST visual seam");
  context.Check(rejected_skips,
                "rejected adapted point reached FAST visual seam");
  CheckClose(context,
             (accepted_candidate.point_w - points_with_var[0].point_w).norm(),
             0.0, "consumer.point_w_semantics");
  CheckClose(context,
             (accepted_candidate.var - points_with_var[0].var)
                 .cwiseAbs()
                 .maxCoeff(),
             0.0, "consumer.var_semantics");
  CheckClose(context,
             (accepted_candidate.normal - points_with_var[0].normal).norm(),
             0.0, "consumer.normal_semantics");
}

}  // namespace

int main() {
  TestContext context;
  TestIdentityAndOrdering(context);
  TestCoordinateAndCovarianceFrames(context);
  TestInputIsolation(context);
  TestRealFastConsumerSemantics(context);
  context.Print("G-I4 pointWithVar corrective semantic gates");
  return context.Passed() ? 0 : 1;
}
