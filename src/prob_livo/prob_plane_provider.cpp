#include "prob_livo/prob_plane_provider.h"

#include <algorithm>
#include <cmath>

namespace prob_livo {

namespace {

// Closed-form largest eigenvalue for a real symmetric 3x3 matrix. This is
// mathematically the same lambda_max used by FAST's support radius, but avoids
// a second Eigen eigensolver after the same HKNN support has already been
// traversed. There is no persistent query state or cache.
double LargestSymmetricEigenvalue(const Eigen::Matrix3d &input) {
  const Eigen::Matrix3d symmetric =
      0.5 * (input + input.transpose());
  const double mean = symmetric.trace() / 3.0;
  const Eigen::Matrix3d centered =
      symmetric - mean * Eigen::Matrix3d::Identity();
  const double p_squared = centered.squaredNorm() / 6.0;
  if (!(p_squared > 0.0)) return mean;

  const double p = std::sqrt(p_squared);
  const double denominator = 2.0 * p * p * p;
  const double determinant_ratio =
      std::max(-1.0, std::min(1.0, centered.determinant() / denominator));
  const double phi = std::acos(determinant_ratio) / 3.0;
  return mean + 2.0 * p * std::cos(phi);
}

}  // namespace

bool ProbPlaneProvider::QueryAtWorldPoint(
    const Eigen::Vector3d &point_W, ProbPlaneQueryResult &result,
    std::string &error) const {
  result = ProbPlaneQueryResult();
  if (!point_W.allFinite()) {
    error = "world query point is non-finite";
    return false;
  }

  ProbPlaneMap::KNNHeapType top_k;
  map_->getTopK(point_W.cast<float>(), top_k);
  if (top_k.count < 4) {
    error = "Prob OctVox returned fewer than four plane supports";
    return false;
  }

  LI2Sup::PlanePointsArray support_points;
  LI2Sup::PlaneCovsArray support_covariances;
  result.support_count = top_k.count;
  result.support_ids.reserve(top_k.count);
  result.support_points_W.reserve(top_k.count);
  result.support_covariances_W.reserve(top_k.count);
  for (int index = 0; index < top_k.count; ++index) {
    support_points[index] = top_k.points_[index].cast<double>();
    support_covariances[index] = top_k.covs_[index];
    result.support_ids.push_back(top_k.ids_[index]);
    result.support_points_W.push_back(support_points[index]);
    result.support_covariances_W.push_back(support_covariances[index]);
  }

  const LI2Sup::PlaneFitQr fit =
      LI2Sup::SolvePlaneFitQr(support_points, top_k.count);
  if (!fit.solved || fit.rank() < 3 || !fit.legacy_accepted) {
    error = "Prob Super QR plane is invalid for this support";
    return false;
  }
  const double q_norm = fit.q.norm();
  result.coeff_nd << fit.q / q_norm, 1.0 / q_norm;
  result.normal_W = result.coeff_nd.head<3>();
  result.d = result.coeff_nd[3];
  result.qr_rank = fit.rank();

  const LI2Sup::ProbQrPlane qr = LI2Sup::ComputeProbQrPlane(
      support_points, support_covariances, top_k.count);
  result.qr_condition = qr.condition;
  if (qr.status == LI2Sup::ProbQrPlane::kValid &&
      qr.covariance.allFinite()) {
    result.plane_cov_nd = qr.covariance;
    result.uncertainty_valid = true;
  }

  Eigen::Vector3d support_mean = Eigen::Vector3d::Zero();
  for (const Eigen::Vector3d &support : result.support_points_W)
    support_mean += support;
  support_mean /= static_cast<double>(result.support_count);
  result.center_W = support_mean -
                    (result.normal_W.dot(support_mean) + result.d) *
                        result.normal_W;

  Eigen::Matrix3d support_covariance = Eigen::Matrix3d::Zero();
  for (const Eigen::Vector3d &support : result.support_points_W) {
    const Eigen::Vector3d delta = support - support_mean;
    support_covariance += delta * delta.transpose();
  }
  support_covariance /= static_cast<double>(result.support_count);
  if (!support_covariance.allFinite()) {
    error = "Prob plane support spread is invalid";
    return false;
  }
  const double largest_eigenvalue =
      LargestSymmetricEigenvalue(support_covariance);
  if (!std::isfinite(largest_eigenvalue)) {
    error = "Prob plane support spread is invalid";
    return false;
  }
  result.radius = std::sqrt(std::max(0.0, largest_eigenvalue));
  result.geometry_valid = true;
  result.valid = true;
  error.clear();
  return true;
}

}  // namespace prob_livo
