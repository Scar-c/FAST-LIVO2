#include "native_offline_reader.h"

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include <ros/ros.h>
#include <rosbag/bag.h>
#include <std_msgs/String.h>

namespace {

constexpr const char *kBag = "/tmp/fast_livo_native_offline_reader_test.bag";

bool Check(bool condition, const std::string &message)
{
  if (!condition) std::cerr << "FAIL: " << message << "\n";
  return condition;
}

void MakeBag()
{
  std::remove(kBag);
  rosbag::Bag bag(kBag, rosbag::bagmode::Write);
  sensor_msgs::Imu imu;
  sensor_msgs::PointCloud2 lidar;
  sensor_msgs::Image image;
  std_msgs::String irrelevant;
  irrelevant.data = "not an estimator topic";

  imu.header.stamp = ros::Time(1.0);
  bag.write("/imu/imu", ros::Time(1.0), imu);
  imu.header.stamp = ros::Time(1.0000001);
  bag.write("/imu/imu", ros::Time(1.0000001), imu);
  lidar.header.stamp = ros::Time(1.1);
  bag.write("/os1_cloud_node1/points", ros::Time(1.1), lidar);
  image.header.stamp = ros::Time(1.15);
  bag.write("/left/image_raw", ros::Time(1.15), image);
  image.header.stamp = ros::Time(1.2);
  bag.write("/left/image_raw", ros::Time(1.2), image);
  lidar.header.stamp = ros::Time(1.3);
  bag.write("/os1_cloud_node1/points", ros::Time(1.3), lidar);
  image.header.stamp = ros::Time(1.4);
  bag.write("/left/image_raw", ros::Time(1.4), image);
  bag.write("/irrelevant", ros::Time(1.25), irrelevant);
  bag.close();
}

bool RunReader(const std::string &image_topic, const std::string &expected,
               std::size_t expected_imu, std::size_t expected_lidar,
               std::size_t expected_image)
{
  fast_livo::NativeOfflineOptions options;
  options.bag_path = kBag;
  options.imu_topic = "/imu/imu";
  options.lidar_topic = "/os1_cloud_node1/points";
  options.image_topic = image_topic;

  std::string events;
  std::size_t process_calls = 0;
  fast_livo::NativeOfflineDispatch dispatch;
  dispatch.on_imu = [&](const sensor_msgs::Imu::ConstPtr &) { events += 'i'; };
  dispatch.on_lidar_pc2 =
      [&](const sensor_msgs::PointCloud2::ConstPtr &) { events += 'l'; };
  dispatch.on_image =
      [&](const sensor_msgs::ImageConstPtr &) { events += 'c'; };
  dispatch.process_available_epochs = [&]() {
    ++process_calls;
    return false;
  };

  fast_livo::NativeOfflineReader reader;
  if (!reader.run(options, dispatch)) return false;
  const auto &accounting = reader.accounting();
  bool ok = true;
  ok &= Check(accounting.imu_read == expected_imu, "IMU record count");
  ok &= Check(accounting.lidar_read == expected_lidar, "LiDAR record count");
  ok &= Check(accounting.image_read == expected_image, "image record count");
  ok &= Check(accounting.ignored_records == 0, "irrelevant topic filtered");
  ok &= Check(process_calls == events.size(), "one source step per dispatch");
  ok &= Check(events == expected, "record-time dispatch order");
  return ok;
}

}  // namespace

int main(int argc, char **argv)
{
  ros::init(argc, argv, "native_offline_reader_test", ros::init_options::NoSigintHandler);
  ros::Time::init();
  MakeBag();
  bool ok = true;

  // IMU burst before LiDAR, a camera epoch between scans, multiple images in
  // one interval, image near EOF, and equal/near-equal timestamps.
  ok &= RunReader("/left/image_raw", "iilcclc", 2, 2, 3);
  // Camera-disabled path consumes no image connection at all.
  ok &= RunReader("", "iill", 2, 2, 0);
  std::remove(kBag);
  return ok ? 0 : 1;
}
