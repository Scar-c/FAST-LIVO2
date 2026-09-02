#include "prob_livo/prob_point_with_var_adapter.h"

namespace prob_livo {

namespace {

Eigen::Matrix3d Skew(const Eigen::Vector3d &point) {
  Eigen::Matrix3d result;
  result << 0.0, -point.z(), point.y(), point.z(), 0.0, -point.x(),
      -point.y(), point.x(), 0.0;
  return result;
}

}  // namespace

bool ProbPointWithVarAdapter::Build(
    std::uint64_t scan_id, const PointCloudXYZI &scan,
    const LI2Sup::VV3 &points_imu,
    const std::vector<LI2Sup::M3d> &sensor_covariances,
    const Eigen::Matrix3d &world_rotation,
    const Eigen::Vector3d &world_translation,
    const Eigen::Matrix3d &lidar_to_imu_rotation,
    const Eigen::Vector3d &lidar_to_imu_translation, std::string &error) {
  output_.points.clear();
  output_.identities.clear();
  output_.normals_available = false;

  if (scan.size() != points_imu.size() ||
      scan.size() != sensor_covariances.size()) {
    error = "scan, IMU point, and sensor covariance counts differ";
    return false;
  }

  output_.points.resize(scan.size());
  output_.identities.resize(scan.size());
  for (std::size_t index = 0; index < scan.size(); ++index) {
    const Eigen::Vector3d point_imu = points_imu[index].cast<double>();
    pointWithVar &point = output_.points[index];
    point.point_i = point_imu;
    point.point_b = lidar_to_imu_rotation.transpose() *
                    (point_imu - lidar_to_imu_translation);
    point.point_w = world_rotation * point_imu + world_translation;
    point.point_crossmat = Skew(point_imu);
    point.body_var = sensor_covariances[index];
    point.var_nostate = world_rotation * point.body_var *
                        world_rotation.transpose();
    point.var = point.var_nostate;
    point.normal.setZero();

    ProbPointIdentity &identity = output_.identities[index];
    identity.scan_id = scan_id;
    identity.source_index = index;
    identity.intensity = scan.points[index].intensity;
    identity.relative_time_ms = scan.points[index].curvature;
  }
  error.clear();
  return true;
}

}  // namespace prob_livo
