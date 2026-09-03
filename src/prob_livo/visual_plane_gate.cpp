#include "prob_livo/visual_plane_gate.h"

#include <Eigen/Eigenvalues>

#include <cmath>

namespace prob_livo {

bool IsFinitePositiveSemidefinite(const Eigen::MatrixXd &covariance,
                                  double tolerance) {
  if (covariance.rows() == 0 || covariance.rows() != covariance.cols() ||
      !covariance.allFinite()) {
    return false;
  }
  const Eigen::MatrixXd symmetric =
      0.5 * (covariance + covariance.transpose());
  if ((covariance - covariance.transpose()).cwiseAbs().maxCoeff() >
      tolerance) {
    return false;
  }
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(symmetric);
  return solver.info() == Eigen::Success && solver.eigenvalues().allFinite() &&
         solver.eigenvalues().minCoeff() >= -tolerance;
}

VisualPlaneGateDecision EvaluateVisualPlaneGate(
    const VisualPlaneGateInput &input, VisualPlaneGateMode mode) {
  VisualPlaneGateDecision decision;
  if (!input.geometry_valid || !std::isfinite(input.radial_distance) ||
      !std::isfinite(input.plane_radius) || input.radial_distance < 0.0 ||
      input.plane_radius < 0.0) {
    return decision;
  }

  // Exact FAST visual radius rule: range_dis <= 3 * plane.radius_.
  decision.radius_gate_pass =
      input.radial_distance <= 3.0 * input.plane_radius;
  if (!decision.radius_gate_pass || !std::isfinite(input.residual)) {
    return decision;
  }

  if (mode == VisualPlaneGateMode::kSuperLegacy) {
    // Exact Super-LIO compute_error rule:
    //     length > 81 * error * error
    // where length is the sensor-relative point range.
    if (std::isfinite(input.sensor_range) && input.sensor_range >= 0.0) {
      decision.second_gate_pass =
          input.sensor_range > 81.0 * input.residual * input.residual;
    }
    return decision;
  }

  if (!input.uncertainty_valid || !input.normal.allFinite() ||
      !IsFinitePositiveSemidefinite(input.plane_covariance) ||
      !IsFinitePositiveSemidefinite(input.point_covariance)) {
    return decision;
  }

  Eigen::Vector4d J_plane;
  J_plane << input.normal, 1.0;
  const double plane_variance =
      (J_plane.transpose() * input.plane_covariance * J_plane)(0, 0);
  const double point_variance =
      (input.normal.transpose() * input.point_covariance * input.normal)(0, 0);
  decision.total_variance = plane_variance + point_variance;
  decision.uncertainty_valid = std::isfinite(plane_variance) &&
                               std::isfinite(point_variance) &&
                               std::isfinite(decision.total_variance) &&
                               plane_variance >= 0.0 && point_variance >= 0.0 &&
                               decision.total_variance >= 0.0;
  if (!decision.uncertainty_valid) return decision;

  // Prompt8 requires the source's strict inequality, not <=.
  decision.second_gate_pass =
      std::abs(input.residual) < 3.0 * std::sqrt(decision.total_variance);
  return decision;
}

Eigen::Vector3d WorldToLidarPoint(const Eigen::Vector3d &point_W,
                                  const Eigen::Matrix3d &R_WI,
                                  const Eigen::Vector3d &p_WI,
                                  const Eigen::Matrix3d &R_IL,
                                  const Eigen::Vector3d &t_IL) {
  const Eigen::Vector3d point_I = R_WI.transpose() * (point_W - p_WI);
  return R_IL.transpose() * (point_I - t_IL);
}

double WorldToLidarRange(const Eigen::Vector3d &point_W,
                         const Eigen::Matrix3d &R_WI,
                         const Eigen::Vector3d &p_WI,
                         const Eigen::Matrix3d &R_IL,
                         const Eigen::Vector3d &t_IL) {
  return WorldToLidarPoint(point_W, R_WI, p_WI, R_IL, t_IL).norm();
}

}  // namespace prob_livo
