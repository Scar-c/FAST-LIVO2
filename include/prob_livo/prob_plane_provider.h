#ifndef PROB_LIVO_PROB_PLANE_PROVIDER_H_
#define PROB_LIVO_PROB_PLANE_PROVIDER_H_

#include "prob_livo/super_native/OctVoxMap/OctVoxMap.hpp"
#include "prob_livo/super_native/prob_qr_plane.h"

#include <Eigen/Core>

#include <cstddef>
#include <string>
#include <vector>

namespace prob_livo {

using ProbPlaneMap = LI2Sup::OctVoxMap<LI2Sup::V3, LI2Sup::scalar>;

struct ProbPlaneQueryResult {
  bool valid = false;
  Eigen::Vector3d normal_W = Eigen::Vector3d::Zero();
  double d = 0.0;
  Eigen::Vector4d coeff_nd = Eigen::Vector4d::Zero();
  Eigen::Vector3d center_W = Eigen::Vector3d::Zero();
  double radius = 0.0;
  std::size_t support_count = 0;
  std::vector<LI2Sup::OctVoxSupportId> support_ids;
  std::vector<Eigen::Vector3d> support_points_W;
  std::vector<Eigen::Matrix3d> support_covariances_W;
  Eigen::Matrix4d plane_cov_nd = Eigen::Matrix4d::Zero();
  int qr_rank = -1;
  double qr_condition = -1.0;
};

// Read-only Prob geometry module. Its interface accepts one explicit world
// point and returns the canonical OctVox/HKNN/QR plane plus diagnostics. The
// map is owned by ProbLioBackend; this module never inserts, clears, or caches.
class ProbPlaneProvider {
 public:
  explicit ProbPlaneProvider(const ProbPlaneMap &map) : map_(&map) {}

  bool QueryAtWorldPoint(const Eigen::Vector3d &point_W,
                         ProbPlaneQueryResult &result,
                         std::string &error) const;

 private:
  const ProbPlaneMap *map_;
};

}  // namespace prob_livo

#endif  // PROB_LIVO_PROB_PLANE_PROVIDER_H_
