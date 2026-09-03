#include "prob_livo/prob_plane_provider.h"

#include <Eigen/Eigenvalues>

#include <algorithm>
#include <cmath>

namespace prob_livo {

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
  const LI2Sup::ProbQrPlane qr = LI2Sup::ComputeProbQrPlane(
      support_points, support_covariances, top_k.count);
  if (qr.status != LI2Sup::ProbQrPlane::kValid) {
    error = "Prob Super QR plane covariance is invalid";
    return false;
  }

  result.coeff_nd = qr.coeff;
  result.normal_W = qr.coeff.head<3>();
  result.d = qr.coeff[3];
  result.plane_cov_nd = qr.covariance;
  result.qr_rank = qr.rank;
  result.qr_condition = qr.condition;

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
  Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eigensolver(
      support_covariance);
  if (eigensolver.info() != Eigen::Success ||
      !eigensolver.eigenvalues().allFinite()) {
    error = "Prob plane support spread is invalid";
    return false;
  }
  result.radius =
      std::sqrt(std::max(0.0, eigensolver.eigenvalues().maxCoeff()));
  result.valid = true;
  error.clear();
  return true;
}

}  // namespace prob_livo
