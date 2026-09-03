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
    const std::vector<LI2Sup::M3d> &lidar_covariances,
    const std::vector<LI2Sup::M3d> &imu_covariances,
    const Eigen::Matrix3d &world_rotation,
    const Eigen::Vector3d &world_translation,
    const Matrix19 &state_covariance,
    const Eigen::Matrix3d &lidar_to_imu_rotation,
    const Eigen::Vector3d &lidar_to_imu_translation,
    const std::vector<std::uint8_t> &accepted_mask,
    const std::vector<Eigen::Vector4d> &plane_coefficients,
    std::string &error) {
  output_.points.clear();
  output_.identities.clear();
  output_.normal_valid.clear();
  output_.normals_available = false;

  if (scan.size() != points_imu.size() ||
      scan.size() != lidar_covariances.size() ||
      scan.size() != imu_covariances.size() ||
      scan.size() != accepted_mask.size() ||
      scan.size() != plane_coefficients.size()) {
    error = "point, covariance, and accepted-plane counts differ";
    return false;
  }

  output_.points.resize(scan.size());
  output_.identities.resize(scan.size());
  output_.normal_valid.assign(scan.size(), 0);
  const Eigen::Matrix3d &rotation_covariance =
      state_covariance.block<3, 3>(Layout::kRot0, Layout::kRot0);
  const Eigen::Matrix3d &position_covariance =
      state_covariance.block<3, 3>(Layout::kPos0, Layout::kPos0);
  for (std::size_t index = 0; index < scan.size(); ++index) {
    const Eigen::Vector3d point_imu = points_imu[index].cast<double>();
    pointWithVar &point = output_.points[index];
    point.point_i = point_imu;
    point.point_b = lidar_to_imu_rotation.transpose() *
                    (point_imu - lidar_to_imu_translation);
    point.point_w = world_rotation * point_imu + world_translation;
    point.point_crossmat = Skew(point_imu);
    point.body_var = lidar_covariances[index];
    point.var_nostate = world_rotation * imu_covariances[index] *
                        world_rotation.transpose();
    point.var = LI2Sup::ComputeMapPointCov(
        point_imu, imu_covariances[index], world_rotation, rotation_covariance,
        position_covariance, LI2Sup::MapPoseCovModel::Livo2Compat);
    if (accepted_mask[index] &&
        plane_coefficients[index].head<3>().allFinite() &&
        plane_coefficients[index].head<3>().norm() > 0.0) {
      point.normal = plane_coefficients[index].head<3>();
      output_.normal_valid[index] = 1;
      output_.normals_available = true;
    }

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
