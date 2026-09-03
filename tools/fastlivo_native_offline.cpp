#include "LIVMapper.h"
#include "native_offline_reader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <image_transport/image_transport.h>

namespace {

bool IsMode(const std::string &mode)
{
  return mode == "lio" || mode == "livo";
}

void WriteOfflineSourceReport(const std::string &output_directory,
                              const fast_livo::NativeOfflineAccounting &accounting,
                              std::size_t drain_steps)
{
  std::ofstream output(output_directory + "/offline_source.yaml");
  output << "schema_version: 1\n"
         << "bag_relevant_messages: " << accounting.bag_relevant_messages << "\n"
         << "lidar_read: " << accounting.lidar_read << "\n"
         << "imu_read: " << accounting.imu_read << "\n"
         << "image_read: " << accounting.image_read << "\n"
         << "ignored_records: " << accounting.ignored_records << "\n"
         << "first_bag_time: " << accounting.first_bag_time << "\n"
         << "last_bag_time: " << accounting.last_bag_time << "\n"
         << "first_sensor_time: " << accounting.first_sensor_time << "\n"
         << "last_sensor_time: " << accounting.last_sensor_time << "\n"
         << "bag_io_wall_s: " << accounting.bag_io_wall_s << "\n"
         << "eof_drain_steps: " << drain_steps << "\n";
}

}  // namespace

int main(int argc, char **argv)
{
  ros::init(argc, argv, "fastlivo_native_offline");
  if (argc != 4 || !IsMode(argv[3])) {
    std::cerr << "usage: fastlivo_native_offline BAG OUTPUT_DIR [lio|livo]\n";
    return 2;
  }

  const std::string bag_path = argv[1];
  const std::string output_directory = argv[2];
  const std::string mode = argv[3];
  std::error_code error;
  std::filesystem::create_directories(output_directory, error);
  if (error) {
    std::cerr << "cannot create output directory: " << error.message() << "\n";
    return 2;
  }

  ros::param::set("/common/img_en", mode == "livo" ? 1 : 0);
  ros::param::set("/common/lidar_en", 1);
  ros::param::set("/common/trajectory_output_path",
                  output_directory + "/trajectory.tum");
  ros::param::set("/evo/pose_output_en", true);
  ros::param::set("/evo/trajectory_output_path",
                  output_directory + "/trajectory.tum");
  ros::param::set("/evo/runtime_report_directory", output_directory);

  ros::NodeHandle nh;
  image_transport::ImageTransport image_transport(nh);
  LIVMapper mapper(nh);
  mapper.initializeSubscribersAndPublishers(nh, image_transport);

  fast_livo::NativeOfflineOptions options;
  options.bag_path = bag_path;
  options.lidar_topic = mapper.lid_topic;
  options.imu_topic = mapper.imu_topic;
  options.image_topic = mapper.img_en ? mapper.img_topic : "";

  fast_livo::NativeOfflineDispatch dispatch;
  dispatch.on_imu = [&](const sensor_msgs::Imu::ConstPtr &message) {
    mapper.imu_cbk(message);
  };
  dispatch.on_image = [&](const sensor_msgs::ImageConstPtr &message) {
    mapper.img_cbk(message);
  };
  if (mapper.p_pre->lidar_type == AVIA) {
    dispatch.on_lidar_livox =
        [&](const livox_ros_driver::CustomMsg::ConstPtr &message) {
          mapper.livox_pcl_cbk(message);
        };
  } else {
    dispatch.on_lidar_pc2 =
        [&](const sensor_msgs::PointCloud2::ConstPtr &message) {
          mapper.standard_pcl_cbk(message);
        };
  }
  dispatch.process_available_epochs = [&]() {
    return mapper.ProcessAvailableNativeEpochs();
  };

  fast_livo::NativeOfflineReader reader;
  if (!reader.run(options, dispatch)) return 1;

  const std::size_t drain_steps = mapper.DrainAvailableNativeEpochs();
  mapper.savePCD();
  mapper.writeRuntimeReports(output_directory);
  WriteOfflineSourceReport(output_directory, reader.accounting(), drain_steps);

  const auto &accounting = reader.accounting();
  std::cout << "[fastlivo_native_offline] mode=" << mode
            << " lidar_read=" << accounting.lidar_read
            << " imu_read=" << accounting.imu_read
            << " image_read=" << accounting.image_read
            << " eof_drain_steps=" << drain_steps
            << " bag_io_wall_s=" << accounting.bag_io_wall_s << "\n";
  ros::shutdown();
  return 0;
}
