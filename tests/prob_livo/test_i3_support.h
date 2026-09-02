#ifndef PROB_LIVO_TEST_I3_SUPPORT_H_
#define PROB_LIVO_TEST_I3_SUPPORT_H_

#include "prob_livo/prob_lio_backend.h"
#include "test_i2_support.h"

#include <Eigen/Geometry>

namespace prob_livo_test {

inline PointType MakePoint(float x, float y, float z, float time_ms = 5.0f) {
  PointType point;
  point.x = x;
  point.y = y;
  point.z = z;
  point.intensity = 1.0f;
  point.curvature = time_ms;
  return point;
}

inline LidarMeasureGroup MakeBackendEpoch(
    double start, double end, const std::vector<prob_livo::ImuSample> &imu,
    const std::vector<PointType> &points,
    const sensor_msgs::Imu::ConstPtr &lookahead = nullptr) {
  LidarMeasureGroup packet;
  packet.last_lio_update_time = start;
  packet.lidar_frame_beg_time = start;
  packet.lidar_frame_end_time = end;
  packet.measures.emplace_back();
  packet.measures.back().lio_time = end;
  for (const auto &sample : imu) packet.measures.back().imu.push_back(ToRosImu(sample));
  packet.pcl_proc_cur->points.clear();
  packet.pcl_proc_cur->points.insert(packet.pcl_proc_cur->points.end(),
                                     points.begin(), points.end());
  packet.pcl_proc_cur->width = points.size();
  packet.pcl_proc_cur->height = 1;
  packet.pcl_proc_cur->is_dense = true;
  packet.lidar = packet.pcl_proc_cur;
  packet.imu_lookahead = lookahead;
  return packet;
}

inline std::vector<PointType> MakePlanePoints(std::size_t count = 24) {
  std::vector<PointType> points;
  points.reserve(count);
  for (std::size_t index = 0; index < count; ++index) {
    const float x = -1.0f + 0.2f * static_cast<float>(index % 10);
    const float y = -0.4f + 0.2f * static_cast<float>(index / 10);
    points.push_back(MakePoint(x, y, 2.0f, 5.0f));
  }
  return points;
}

inline prob_livo::ProbLioBackend::Options TestBackendOptions() {
  prob_livo::ProbLioBackend::Options options;
  options.map_resolution = 0.5;
  options.voxel_size = 0.5;
  options.lidar_depth_error = 0.02;
  options.lidar_beam_error_deg = 0.01;
  options.map_capacity = 10000;
  options.trajectory_path.clear();
  return options;
}

}  // namespace prob_livo_test

#endif  // PROB_LIVO_TEST_I3_SUPPORT_H_
