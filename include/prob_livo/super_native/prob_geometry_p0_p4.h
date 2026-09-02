#ifndef PROB_LIVO_SUPER_NATIVE_PROB_GEOMETRY_P0_P4_H_
#define PROB_LIVO_SUPER_NATIVE_PROB_GEOMETRY_P0_P4_H_

// P0-P4 copy-on-demand geometry core from Super-LIO's production tree.
// P5 association and the standalone ROS wrapper are intentionally excluded.

#include "basic/alias.h"

#include <Eigen/Core>
#include <Eigen/Eigenvalues>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace LI2Sup {

// Keep the Super-native namespace spelling used by the copied map and the
// backend while retaining BASIC as the single type-definition authority.
using scalar = BASIC::scalar;
using V3 = BASIC::V3;
using V3d = BASIC::V3d;
using VV3 = BASIC::VV3;
using M3d = BASIC::M3d;

inline void CalcLidarPointCov(const BASIC::V3d &point_in,
                              double range_increment,
                              double beam_increment_deg,
                              BASIC::M3d &covariance) {
  // Same float narrowing and DEG2RAD constant as FAST-LIVO2's active
  // voxel_map.cpp calcBodyCov implementation.
  BASIC::V3d point = point_in;
  const float range_inc = static_cast<float>(range_increment);
  const float degree_inc = static_cast<float>(beam_increment_deg);
  if (point[2] == 0.0) point[2] = 0.0001;
  const float range = std::sqrt(point[0] * point[0] + point[1] * point[1] +
                                point[2] * point[2]);
  const float range_var = range_inc * range_inc;
  const double angle = static_cast<double>(degree_inc) * 0.017453293;
  Eigen::Matrix2d direction_var;
  direction_var << std::pow(std::sin(angle), 2), 0, 0,
      std::pow(std::sin(angle), 2);
  BASIC::V3d direction(point);
  direction.normalize();
  BASIC::M3d direction_hat;
  direction_hat << 0, -direction(2), direction(1), direction(2), 0,
      -direction(0), -direction(1), direction(0), 0;
  BASIC::V3d base_vector1(1, 1,
                          -(direction(0) + direction(1)) / direction(2));
  base_vector1.normalize();
  BASIC::V3d base_vector2 = base_vector1.cross(direction);
  base_vector2.normalize();
  Eigen::Matrix<double, 3, 2> basis;
  basis << base_vector1(0), base_vector2(0), base_vector1(1), base_vector2(1),
      base_vector1(2), base_vector2(2);
  const Eigen::Matrix<double, 3, 2> A = range * direction_hat * basis;
  covariance = direction * range_var * direction.transpose() +
               A * direction_var * A.transpose();
}

enum class CovValidationMode { Light = 0, Full = 1 };

inline bool CovarianceIsFiniteSymmetric(const BASIC::M3d &covariance,
                                        double tolerance = 1e-9) {
  return covariance.allFinite() &&
         (covariance - covariance.transpose()).norm() <= tolerance;
}

inline bool CovarianceIsValid(const BASIC::M3d &covariance,
                              double tolerance = 1e-9) {
  if (!CovarianceIsFiniteSymmetric(covariance, tolerance)) return false;
  Eigen::SelfAdjointEigenSolver<BASIC::M3d> solver(covariance);
  return solver.info() == Eigen::Success &&
         solver.eigenvalues().minCoeff() >= -tolerance;
}

inline bool ValidateCovariance(const BASIC::M3d &covariance,
                              CovValidationMode mode, double tolerance = 1e-9) {
  return mode == CovValidationMode::Full
             ? CovarianceIsValid(covariance, tolerance)
             : CovarianceIsFiniteSymmetric(covariance, tolerance);
}

inline BASIC::M3d RotateCovariance(const BASIC::M3d &rotation,
                                   const BASIC::M3d &covariance) {
  return rotation * covariance * rotation.transpose();
}

enum class CovStoragePrecision { Double = 0, FloatQuantized = 1 };

enum class MapPoseCovModel { Livo2Compat = 0, SuperRightConsistent = 1 };

inline BASIC::M3d SkewSymmetric(const BASIC::V3d &value) {
  BASIC::M3d result;
  result << 0.0, -value(2), value(1), value(2), 0.0, -value(0), -value(1),
      value(0), 0.0;
  return result;
}

inline BASIC::M3d ComputeMapPointCov(const BASIC::V3d &point_imu,
                                     const BASIC::M3d &sensor_covariance,
                                     const BASIC::M3d &world_rotation,
                                     const BASIC::M3d &rotation_covariance,
                                     const BASIC::M3d &position_covariance,
                                     MapPoseCovModel model =
                                         MapPoseCovModel::Livo2Compat) {
  const BASIC::M3d skew = SkewSymmetric(point_imu);
  const BASIC::M3d rotation_term =
      model == MapPoseCovModel::SuperRightConsistent
          ? RotateCovariance(world_rotation * skew, rotation_covariance)
          : skew * rotation_covariance * skew.transpose();
  return RotateCovariance(world_rotation, sensor_covariance) + rotation_term +
         position_covariance;
}

inline void ComputeMapCovList(
    const BASIC::VV3 &points_imu,
    const std::vector<BASIC::M3d> &sensor_covariances,
    const BASIC::M3d &world_rotation, const BASIC::M3d &rotation_covariance,
    const BASIC::M3d &position_covariance,
    std::vector<BASIC::M3d> &world_covariances,
    MapPoseCovModel model = MapPoseCovModel::Livo2Compat) {
  world_covariances.resize(points_imu.size());
  for (std::size_t i = 0; i < points_imu.size(); ++i) {
    world_covariances[i] = ComputeMapPointCov(
        points_imu[i].cast<double>(), sensor_covariances[i], world_rotation,
        rotation_covariance, position_covariance, model);
  }
}

inline void ComputeInitMapCovList(
    const BASIC::VV3 &points_lidar, const BASIC::M3d &lidar_to_imu_rotation,
    const BASIC::V3d &lidar_to_imu_translation, double depth_error,
    double beam_error_deg, const BASIC::M3d &world_rotation,
    const BASIC::M3d &rotation_covariance,
    const BASIC::M3d &position_covariance,
    std::vector<BASIC::M3d> &world_covariances,
    MapPoseCovModel model = MapPoseCovModel::Livo2Compat) {
  world_covariances.resize(points_lidar.size());
  for (std::size_t i = 0; i < points_lidar.size(); ++i) {
    BASIC::M3d lidar_covariance;
    CalcLidarPointCov(points_lidar[i].cast<double>(), depth_error,
                      beam_error_deg, lidar_covariance);
    const BASIC::V3d point_imu =
        lidar_to_imu_rotation * points_lidar[i].cast<double>() +
        lidar_to_imu_translation;
    world_covariances[i] = ComputeMapPointCov(
        point_imu, RotateCovariance(lidar_to_imu_rotation, lidar_covariance),
        world_rotation, rotation_covariance, position_covariance, model);
  }
}

inline void ComputeBodyCovListWithExtrinsic(
    const BASIC::VV3 &points_imu, const BASIC::M3d &lidar_to_imu_rotation,
    const BASIC::V3d &lidar_to_imu_translation, double depth_error,
    double beam_error_deg, std::vector<BASIC::M3d> &covariances_imu) {
  covariances_imu.resize(points_imu.size());
  for (std::size_t i = 0; i < points_imu.size(); ++i) {
    const BASIC::V3d point_lidar =
        lidar_to_imu_rotation.transpose() *
        (points_imu[i].cast<double>() - lidar_to_imu_translation);
    BASIC::M3d lidar_covariance;
    CalcLidarPointCov(point_lidar, depth_error, beam_error_deg,
                      lidar_covariance);
    covariances_imu[i] = RotateCovariance(lidar_to_imu_rotation,
                                           lidar_covariance);
  }
}

enum class P2pWeightMode { Fixed1000 = 0, ProbLivo2 = 1 };

inline double PlaneResidualVariance(const BASIC::V3d &point_world,
                                    const Eigen::Matrix4d &plane_covariance) {
  Eigen::Vector4d jacobian;
  jacobian << point_world, 1.0;
  return jacobian.dot(plane_covariance * jacobian);
}

inline double PointResidualVariance(const BASIC::V3d &normal,
                                    const BASIC::M3d &world_rotation,
                                    const BASIC::M3d &sensor_covariance) {
  const BASIC::V3d rotated_normal = world_rotation.transpose() * normal;
  return rotated_normal.dot(sensor_covariance * rotated_normal);
}

struct ProbWeight {
  bool valid = false;
  bool invalid_nonfinite = false;
  bool invalid_negative = false;
  double weight = 0.0;
};

inline ProbWeight ComputeP2pProbWeight(double plane_variance,
                                       double point_variance,
                                       double floor = 0.001) {
  ProbWeight result;
  if (!std::isfinite(plane_variance) || !std::isfinite(point_variance) ||
      !std::isfinite(floor)) {
    result.invalid_nonfinite = true;
    return result;
  }
  constexpr double kNegativeEpsilon = 1e-9;
  if (plane_variance < 0.0 && plane_variance > -kNegativeEpsilon)
    plane_variance = 0.0;
  if (point_variance < 0.0 && point_variance > -kNegativeEpsilon)
    point_variance = 0.0;
  if (plane_variance < 0.0 || point_variance < 0.0) {
    result.invalid_negative = true;
    return result;
  }
  const double residual_variance = floor + plane_variance + point_variance;
  if (!std::isfinite(residual_variance) || residual_variance <= 0.0) {
    result.invalid_negative = true;
    return result;
  }
  result.weight = 1.0 / residual_variance;
  result.valid = true;
  return result;
}

}  // namespace LI2Sup

#endif  // PROB_LIVO_SUPER_NATIVE_PROB_GEOMETRY_P0_P4_H_
