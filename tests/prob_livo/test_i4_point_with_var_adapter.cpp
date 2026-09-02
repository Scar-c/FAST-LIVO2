#include "prob_livo/prob_point_with_var_adapter.h"
#include "test_i3_support.h"
#include "vio.h"

#include <Eigen/Geometry>

#include <cmath>
#include <string>
#include <vector>

namespace {

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

std::vector<LI2Sup::M3d> MakeCovariances() {
  std::vector<LI2Sup::M3d> covariances(2, LI2Sup::M3d::Zero());
  covariances[0] << 0.04, 0.01, -0.003, 0.01, 0.09, 0.006, -0.003, 0.006,
      0.16;
  covariances[1] << 0.02, -0.004, 0.002, -0.004, 0.07, 0.005, 0.002, 0.005,
      0.11;
  return covariances;
}

void CheckClose(TestContext &context, double error, double tolerance,
                const std::string &name) {
  context.Record(name, error);
  context.Check(error <= tolerance, name + " mismatch");
}

void TestIdentityAndOrdering(TestContext &context) {
  const PointCloudXYZI scan = MakeScan();
  const LI2Sup::VV3 points = MakeImuPoints();
  const std::vector<LI2Sup::M3d> covariances = MakeCovariances();
  ProbPointWithVarAdapter adapter;
  std::string error;
  const bool ok = adapter.Build(
      42, scan, points, covariances, Eigen::Matrix3d::Identity(),
      Eigen::Vector3d::Zero(), Eigen::Matrix3d::Identity(),
      Eigen::Vector3d::Zero(), error);
  context.Check(ok, "identity adapter build failed: " + error);
  if (!ok) return;

  const ProbPointWithVarBuffer &output = adapter.output();
  context.Check(output.points.size() == 2, "identity output count mismatch");
  context.Check(output.identities.size() == 2, "identity metadata count mismatch");
  context.Check(!output.normals_available, "normal capability must be unavailable");
  for (std::size_t index = 0; index < output.points.size(); ++index) {
    const pointWithVar &point = output.points[index];
    const ProbPointIdentity &identity = output.identities[index];
    CheckClose(context, (point.point_i - points[index].cast<double>()).norm(), 0.0,
               "identity.point_i");
    CheckClose(context, (point.point_b - points[index].cast<double>()).norm(), 0.0,
               "identity.point_b");
    CheckClose(context, (point.point_w - points[index].cast<double>()).norm(), 0.0,
               "identity.point_w");
    CheckClose(context, (point.body_var - covariances[index]).cwiseAbs().maxCoeff(),
               0.0, "identity.body_var");
    CheckClose(context, (point.var - covariances[index]).cwiseAbs().maxCoeff(), 0.0,
               "identity.world_var");
    CheckClose(context, (point.var_nostate - covariances[index]).cwiseAbs().maxCoeff(),
               0.0, "identity.var_nostate");
    CheckClose(context, point.normal.norm(), 0.0, "identity.normal_zero");
    context.Check(identity.scan_id == 42, "identity scan id mismatch");
    context.Check(identity.source_index == index, "identity source order mismatch");
    context.Check(identity.intensity == scan.points[index].intensity,
                  "identity intensity mismatch");
    context.Check(identity.relative_time_ms == scan.points[index].curvature,
                  "identity relative time mismatch");
  }

  const bool bad_size = adapter.Build(
      42, scan, LI2Sup::VV3(1, LI2Sup::V3::Zero()), covariances,
      Eigen::Matrix3d::Identity(), Eigen::Vector3d::Zero(),
      Eigen::Matrix3d::Identity(), Eigen::Vector3d::Zero(), error);
  context.Check(!bad_size && !error.empty(), "size mismatch must be rejected");
}

void TestCoordinateAndCovarianceFrames(TestContext &context) {
  const PointCloudXYZI scan = MakeScan();
  const LI2Sup::VV3 points = MakeImuPoints();
  const std::vector<LI2Sup::M3d> covariances = MakeCovariances();
  const Eigen::Matrix3d world_rotation =
      Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d::UnitZ())
          .toRotationMatrix();
  const Eigen::Vector3d world_translation(10.0, -2.0, 0.5);
  const Eigen::Matrix3d lidar_to_imu_rotation =
      Eigen::AngleAxisd(0.3, Eigen::Vector3d::UnitY()).toRotationMatrix();
  const Eigen::Vector3d lidar_to_imu_translation(0.2, -0.1, 0.3);

  ProbPointWithVarAdapter adapter;
  std::string error;
  const bool ok = adapter.Build(
      7, scan, points, covariances, world_rotation, world_translation,
      lidar_to_imu_rotation, lidar_to_imu_translation, error);
  context.Check(ok, "frame adapter build failed: " + error);
  if (!ok) return;

  const pointWithVar &first = adapter.output().points.front();
  const Eigen::Vector3d point_imu(1.0, 2.0, 3.0);
  const Eigen::Vector3d expected_world =
      world_rotation * point_imu + world_translation;
  const Eigen::Vector3d expected_lidar =
      lidar_to_imu_rotation.transpose() *
      (point_imu - lidar_to_imu_translation);
  const Eigen::Matrix3d expected_world_covariance =
      world_rotation * covariances.front() * world_rotation.transpose();
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
             (first.body_var - covariances.front()).cwiseAbs().maxCoeff(),
             1e-12, "frames.sensor_covariance");
  CheckClose(context,
             (first.var - expected_world_covariance).cwiseAbs().maxCoeff(),
             1e-12, "frames.world_covariance");
  CheckClose(context,
             (first.var_nostate - expected_world_covariance).cwiseAbs().maxCoeff(),
             1e-12, "frames.world_sensor_covariance");

  const Eigen::Matrix3d wrong_direction =
      world_rotation.transpose() * covariances.front() * world_rotation;
  const Eigen::Matrix3d double_rotated =
      world_rotation * expected_world_covariance * world_rotation.transpose();
  context.Check((first.var - wrong_direction).cwiseAbs().maxCoeff() > 1e-4,
                "negative covariance rotation direction was not detected");
  context.Check((first.var - double_rotated).cwiseAbs().maxCoeff() > 1e-4,
                "negative double covariance rotation was not detected");
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
  const std::vector<LI2Sup::M3d> covariances = MakeCovariances();
  const std::vector<LI2Sup::M3d> covariances_before = covariances;

  ProbPointWithVarAdapter adapter;
  std::string error;
  const bool ok = adapter.Build(
      99, scan, points, covariances, Eigen::Matrix3d::Identity(),
      Eigen::Vector3d::Zero(), Eigen::Matrix3d::Identity(),
      Eigen::Vector3d::Zero(), error);
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
               (covariances[index] - covariances_before[index])
                   .cwiseAbs()
                   .maxCoeff(),
               0.0, "input-isolation.covariance");
  }
}

void TestRealFastConsumerType(TestContext &context) {
  using ProcessFrame = void (VIOManager::*)(
      cv::Mat &, std::vector<pointWithVar> &,
      const std::unordered_map<VOXEL_LOCATION, VoxelOctoTree *> &, double);
  ProcessFrame process_frame = &VIOManager::processFrame;
  context.Check(process_frame != nullptr,
                "pointWithVar is not the real VIO processFrame input type");
}

}  // namespace

int main() {
  TestContext context;
  TestIdentityAndOrdering(context);
  TestCoordinateAndCovarianceFrames(context);
  TestInputIsolation(context);
  TestRealFastConsumerType(context);
  context.Print("G-I4 pointWithVar current-scan adapter gates");
  return context.Passed() ? 0 : 1;
}
