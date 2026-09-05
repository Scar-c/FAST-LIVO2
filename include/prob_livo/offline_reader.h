#ifndef PROB_LIVO_OFFLINE_READER_H_
#define PROB_LIVO_OFFLINE_READER_H_

// Transport-only ROS1 bag reader for the production FAST/LIVO2 shell.
// Estimator synchronization remains owned by LIVMapper::sync_packages(); the
// reader only dispatches relevant messages in bag-record order and invokes one
// estimator step after each arrival.

#include <cstddef>
#include <functional>
#include <string>

#include <livox_ros_driver/CustomMsg.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/PointCloud2.h>

namespace prob_livo {

struct OfflineOptions {
  std::string bag_path;
  std::string lidar_topic;
  std::string imu_topic;
  std::string image_topic;
  std::function<void(double)> sensor_progress;
  double start_offset = -1.0;
  double duration = -1.0;
};

struct OfflineAccounting {
  std::size_t bag_relevant_messages = 0;
  std::size_t lidar_read = 0;
  std::size_t imu_read = 0;
  std::size_t image_read = 0;
  std::size_t other_messages = 0;
  double first_bag_time = 0.0;
  double last_bag_time = 0.0;
  double first_sensor_time = 0.0;
  double last_sensor_time = 0.0;
  double wall_processing_s = 0.0;
  double image_decode_s = 0.0;
  std::size_t image_decode_failures = 0;
  double sensor_duration_s = 0.0;
  double speed_factor = 0.0;
};

struct OfflineDispatch {
  std::function<void(const sensor_msgs::Imu::ConstPtr &)> on_imu;
  std::function<void(const sensor_msgs::ImageConstPtr &)> on_image;
  std::function<void(const sensor_msgs::PointCloud2::ConstPtr &)> on_lidar_pc2;
  std::function<void(const livox_ros_driver::CustomMsg::ConstPtr &)> on_lidar_livox;
  std::function<void()> step;
};

class OfflineReader {
 public:
  bool run(const OfflineOptions &options, const OfflineDispatch &dispatch);
  const OfflineAccounting &accounting() const { return accounting_; }

 private:
  OfflineAccounting accounting_;
};

}  // namespace prob_livo

#endif  // PROB_LIVO_OFFLINE_READER_H_
