#ifndef PROB_LIVO_PROB_POINT_WITH_VAR_ADAPTER_H_
#define PROB_LIVO_PROB_POINT_WITH_VAR_ADAPTER_H_

#include "common_lib.h"
#include "prob_livo/prob_eskf19.h"
#include "prob_livo/super_native/prob_geometry_p0_p4.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace prob_livo {

// Sidecar identity for the temporary current-scan buffer.  pointWithVar has
// no intensity or relative-time fields, so the adapter keeps those metadata
// values without making them part of the FAST-owned type.
struct ProbPointIdentity {
  std::uint64_t scan_id = 0;
  std::size_t source_index = 0;
  float intensity = 0.0f;
  float relative_time_ms = 0.0f;
};

struct ProbPointWithVarBuffer {
  std::vector<pointWithVar> points;
  std::vector<ProbPointIdentity> identities;
  std::vector<std::uint8_t> normal_valid;
  bool normals_available = false;
};

// Converts the authoritative Prob current scan into FAST's pointWithVar
// consumer contract.  It owns only a replaceable temporary scan buffer; it
// does not own filtering, state, map, plane, feature, or trajectory logic.
class ProbPointWithVarAdapter {
 public:
  bool Build(std::uint64_t scan_id, const PointCloudXYZI &scan,
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
             std::string &error);

  const ProbPointWithVarBuffer &output() const { return output_; }

 private:
  ProbPointWithVarBuffer output_;
};

}  // namespace prob_livo

#endif  // PROB_LIVO_PROB_POINT_WITH_VAR_ADAPTER_H_
