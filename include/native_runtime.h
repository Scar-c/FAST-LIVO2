#ifndef FAST_LIVO_NATIVE_RUNTIME_H_
#define FAST_LIVO_NATIVE_RUNTIME_H_

#include <cstdint>

// Runtime accounting is deliberately kept at production seams.  It contains
// no estimator state and is shared by the ROS and rosbag event sources.
struct NativeRuntimeCounters
{
  std::uint64_t imu_callbacks_received = 0;
  std::uint64_t lidar_callbacks_received = 0;
  std::uint64_t image_callbacks_received = 0;
  std::uint64_t imu_messages_enqueued = 0;
  std::uint64_t lidar_messages_enqueued = 0;
  std::uint64_t image_messages_enqueued = 0;
  std::uint64_t ignored_input_messages = 0;

  // A step is a successfully synchronized native epoch.  Poll attempts are
  // source-specific because online ROS spins also poll empty queues.
  std::uint64_t scheduler_step_calls = 0;
  std::uint64_t scheduler_poll_calls = 0;
  std::uint64_t scheduler_sync_packages = 0;
  std::uint64_t lidar_epochs = 0;
  std::uint64_t camera_epochs = 0;

  std::uint64_t imu_processing_calls = 0;
  std::uint64_t lidar_update_calls = 0;
  std::uint64_t map_query_calls = 0;
  std::uint64_t map_update_calls = 0;
  std::uint64_t visual_process_calls = 0;
  std::uint64_t visual_state_commits = 0;
  std::uint64_t trajectory_rows = 0;
};

struct NativeRuntimeTiming
{
  double input_preprocess_s = 0.0;
  double imu_s = 0.0;
  double lidar_association_update_s = 0.0;
  double map_query_s = 0.0;
  double map_update_s = 0.0;
  double visual_processing_s = 0.0;
  double estimator_compute_s = 0.0;

  std::uint64_t input_preprocess_count = 0;
  std::uint64_t imu_count = 0;
  std::uint64_t lidar_association_update_count = 0;
  std::uint64_t map_query_count = 0;
  std::uint64_t map_update_count = 0;
  std::uint64_t visual_processing_count = 0;
  std::uint64_t estimator_compute_count = 0;
};

#endif  // FAST_LIVO_NATIVE_RUNTIME_H_
