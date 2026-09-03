#ifndef PROB_LIVO_VISUAL_PLANE_GATE_H_
#define PROB_LIVO_VISUAL_PLANE_GATE_H_

#include <Eigen/Core>

namespace prob_livo {

// This is the only policy switch used by the camera-on ablation.  The FAST
// radius gate is evaluated first and is deliberately shared by both modes.
enum class VisualPlaneGateMode {
  kLivo2Prob3sigma,
  kSuperLegacy,
};

struct VisualPlaneGateInput {
  bool geometry_valid = false;
  bool uncertainty_valid = false;
  double residual = 0.0;
  double sensor_range = 0.0;
  double radial_distance = 0.0;
  double plane_radius = 0.0;
  Eigen::Vector3d normal = Eigen::Vector3d::Zero();
  Eigen::Matrix3d point_covariance = Eigen::Matrix3d::Zero();
  Eigen::Matrix4d plane_covariance = Eigen::Matrix4d::Zero();
};

struct VisualPlaneGateDecision {
  bool radius_gate_pass = false;
  bool second_gate_pass = false;
  bool uncertainty_valid = false;
  double total_variance = 0.0;
};

bool IsFinitePositiveSemidefinite(const Eigen::MatrixXd &covariance,
                                  double tolerance = 1e-10);

VisualPlaneGateDecision EvaluateVisualPlaneGate(
    const VisualPlaneGateInput &input, VisualPlaneGateMode mode);

// Convert a world point to the LiDAR sensor frame.  R_I_L and t_I_L encode
// p_I = R_I_L * p_L + t_I_L, matching FAST's extrinsic convention.
Eigen::Vector3d WorldToLidarPoint(const Eigen::Vector3d &point_W,
                                  const Eigen::Matrix3d &R_WI,
                                  const Eigen::Vector3d &p_WI,
                                  const Eigen::Matrix3d &R_IL,
                                  const Eigen::Vector3d &t_IL);

double WorldToLidarRange(const Eigen::Vector3d &point_W,
                         const Eigen::Matrix3d &R_WI,
                         const Eigen::Vector3d &p_WI,
                         const Eigen::Matrix3d &R_IL,
                         const Eigen::Vector3d &t_IL);

}  // namespace prob_livo

#endif  // PROB_LIVO_VISUAL_PLANE_GATE_H_
