#ifndef PROB_LIVO_INPUT_SEMANTICS_H_
#define PROB_LIVO_INPUT_SEMANTICS_H_

#include "common_lib.h"

#include <string>

namespace prob_livo {

enum class InputSemantics { kFastNative, kSuperNtuLegacy };

inline bool ParseInputSemantics(const std::string &name,
                                InputSemantics &semantics) {
  if (name == "fast_native") {
    semantics = InputSemantics::kFastNative;
    return true;
  }
  if (name == "super_ntu_legacy") {
    semantics = InputSemantics::kSuperNtuLegacy;
    return true;
  }
  return false;
}

inline double LidarHeaderTime(InputSemantics semantics, double ros_stamp,
                              double configured_offset) {
  (void)semantics;
  return ros_stamp + configured_offset;
}

inline double LidarEndTime(const PointCloudXYZI &cloud, double header_time) {
  if (cloud.empty()) return header_time;
  return header_time + static_cast<double>(cloud.points.back().curvature) /
      1000.0;
}

}  // namespace prob_livo

#endif  // PROB_LIVO_INPUT_SEMANTICS_H_
