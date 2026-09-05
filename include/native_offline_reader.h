#ifndef FAST_LIVO_NATIVE_OFFLINE_READER_H_
#define FAST_LIVO_NATIVE_OFFLINE_READER_H_

#include <cstddef>
#include <functional>
#include <string>

#include <livox_ros_driver/CustomMsg.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/PointCloud2.h>

namespace fast_livo {

struct NativeOfflineOptions
{
  std::string bag_path;
  std::string lidar_topic;
  std::string imu_topic;
  std::string image_topic;
  std::function<void(double)> sensor_progress;
};

struct NativeOfflineAccounting
{
  std::size_t bag_relevant_messages = 0;
  std::size_t lidar_read = 0;
  std::size_t imu_read = 0;
  std::size_t image_read = 0;
  std::size_t ignored_records = 0;
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

struct NativeOfflineDispatch
{
  std::function<void(const sensor_msgs::Imu::ConstPtr &)> on_imu;
  std::function<void(const sensor_msgs::ImageConstPtr &)> on_image;
  std::function<void(const sensor_msgs::PointCloud2::ConstPtr &)> on_lidar_pc2;
  std::function<void(const livox_ros_driver::CustomMsg::ConstPtr &)> on_lidar_livox;
  std::function<bool()> process_available_epochs;
};

class NativeOfflineReader
{
 public:
  bool run(const NativeOfflineOptions &options,
           const NativeOfflineDispatch &dispatch);
  const NativeOfflineAccounting &accounting() const { return accounting_; }

 private:
  NativeOfflineAccounting accounting_;
};

}  // namespace fast_livo

#endif  // FAST_LIVO_NATIVE_OFFLINE_READER_H_
