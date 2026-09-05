#include "prob_livo/offline_reader.h"

#include <chrono>
#include <cstdio>
#include <exception>
#include <vector>

#include <cv_bridge/cv_bridge.h>
#include <opencv2/imgcodecs.hpp>
#include <rosbag/bag.h>
#include <rosbag/view.h>
#include <sensor_msgs/CompressedImage.h>
#include <sensor_msgs/image_encodings.h>

namespace prob_livo {

namespace {

double WallTimeSeconds() {
  return std::chrono::duration<double>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

template <typename Message>
void RecordSensorTime(const Message &message, OfflineAccounting &accounting) {
  const double timestamp = message.header.stamp.toSec();
  if (accounting.first_sensor_time == 0.0) {
    accounting.first_sensor_time = timestamp;
  }
  accounting.last_sensor_time = timestamp;
}

sensor_msgs::ImagePtr DecodeCompressedImage(
    const sensor_msgs::CompressedImage::ConstPtr &compressed) {
  if (!compressed || compressed->data.empty()) return {};
  const cv::Mat encoded(1, static_cast<int>(compressed->data.size()), CV_8UC1,
                        const_cast<unsigned char *>(compressed->data.data()));
  const cv::Mat decoded = cv::imdecode(encoded, cv::IMREAD_COLOR);
  if (decoded.empty()) return {};
  return cv_bridge::CvImage(compressed->header,
                            sensor_msgs::image_encodings::BGR8, decoded)
      .toImageMsg();
}

}  // namespace

bool OfflineReader::run(const OfflineOptions &options,
                        const OfflineDispatch &dispatch) {
  accounting_ = OfflineAccounting();
  if (options.bag_path.empty() || options.lidar_topic.empty() ||
      options.imu_topic.empty() || !dispatch.on_imu || !dispatch.step ||
      (!options.image_topic.empty() && !dispatch.on_image)) {
    std::printf("[Prob-LIVO OfflineReader] ERROR: invalid options/dispatch\n");
    return false;
  }

  rosbag::Bag bag;
  try {
    bag.open(options.bag_path, rosbag::bagmode::Read);
  } catch (const std::exception &error) {
    std::printf("[Prob-LIVO OfflineReader] ERROR: cannot open bag %s: %s\n",
                options.bag_path.c_str(), error.what());
    return false;
  }

  rosbag::View view;
  try {
    std::vector<std::string> topics{options.lidar_topic, options.imu_topic};
    if (!options.image_topic.empty()) topics.push_back(options.image_topic);
    view.addQuery(bag, rosbag::TopicQuery(topics));
  } catch (const std::exception &error) {
    std::printf("[Prob-LIVO OfflineReader] ERROR: view query failed: %s\n",
                error.what());
    return false;
  }

  const double view_begin = view.getBeginTime().toSec();
  const double view_end = view.getEndTime().toSec();
  const double begin = options.start_offset >= 0.0
                           ? view_begin + options.start_offset
                           : view_begin;
  const double end = options.duration >= 0.0 ? begin + options.duration
                                             : view_end;
  const double wall_begin = WallTimeSeconds();
  bool first_record = true;

  for (const rosbag::MessageInstance &instance : view) {
    const double record_time = instance.getTime().toSec();
    if (record_time < begin || record_time > end) continue;

    ++accounting_.bag_relevant_messages;
    if (first_record) {
      accounting_.first_bag_time = record_time;
      first_record = false;
    }
    accounting_.last_bag_time = record_time;

    const std::string &topic = instance.getTopic();
    const std::string &datatype = instance.getDataType();
    if (topic == options.imu_topic && datatype == "sensor_msgs/Imu") {
      const auto message = instance.instantiate<sensor_msgs::Imu>();
      if (message) {
        ++accounting_.imu_read;
        RecordSensorTime(*message, accounting_);
        if (options.sensor_progress)
          options.sensor_progress(message->header.stamp.toSec());
        dispatch.on_imu(message);
        dispatch.step();
        continue;
      }
    }

    if (!options.image_topic.empty() && topic == options.image_topic) {
      sensor_msgs::ImageConstPtr message;
      if (datatype == "sensor_msgs/Image") {
        message = instance.instantiate<sensor_msgs::Image>();
      } else if (datatype == "sensor_msgs/CompressedImage") {
        const auto compressed =
            instance.instantiate<sensor_msgs::CompressedImage>();
        const double decode_begin = WallTimeSeconds();
        message = DecodeCompressedImage(compressed);
        accounting_.image_decode_s += WallTimeSeconds() - decode_begin;
        if (!message) ++accounting_.image_decode_failures;
      }
      if (message) {
        ++accounting_.image_read;
        RecordSensorTime(*message, accounting_);
        if (options.sensor_progress)
          options.sensor_progress(message->header.stamp.toSec());
        dispatch.on_image(message);
        dispatch.step();
        continue;
      }
    }

    if (topic == options.lidar_topic && datatype == "sensor_msgs/PointCloud2") {
      const auto message = instance.instantiate<sensor_msgs::PointCloud2>();
      if (message) {
        ++accounting_.lidar_read;
        RecordSensorTime(*message, accounting_);
        if (options.sensor_progress)
          options.sensor_progress(message->header.stamp.toSec());
        if (dispatch.on_lidar_pc2) dispatch.on_lidar_pc2(message);
        dispatch.step();
        continue;
      }
    }

    if (topic == options.lidar_topic &&
        datatype == "livox_ros_driver/CustomMsg") {
      const auto message =
          instance.instantiate<livox_ros_driver::CustomMsg>();
      if (message) {
        ++accounting_.lidar_read;
        RecordSensorTime(*message, accounting_);
        if (options.sensor_progress)
          options.sensor_progress(message->header.stamp.toSec());
        if (dispatch.on_lidar_livox) dispatch.on_lidar_livox(message);
        dispatch.step();
        continue;
      }
    }

    ++accounting_.other_messages;
  }

  accounting_.wall_processing_s = WallTimeSeconds() - wall_begin;
  accounting_.sensor_duration_s = accounting_.last_sensor_time -
                                  accounting_.first_sensor_time;
  accounting_.speed_factor = accounting_.wall_processing_s > 0.0
                                 ? accounting_.sensor_duration_s /
                                       accounting_.wall_processing_s
                                 : 0.0;
  bag.close();

  std::printf(
      "[Prob-LIVO OfflineReader] view %.3fs..%.3fs lidar=%zu imu=%zu image=%zu "
      "other=%zu wall=%.3fs decode=%.3fs decode_failures=%zu sensor=%.3fs "
      "speed=%.3fx\n",
      view_begin, view_end, accounting_.lidar_read, accounting_.imu_read,
      accounting_.image_read, accounting_.other_messages,
      accounting_.wall_processing_s, accounting_.image_decode_s,
      accounting_.image_decode_failures,
      accounting_.sensor_duration_s, accounting_.speed_factor);
  return true;
}

}  // namespace prob_livo
