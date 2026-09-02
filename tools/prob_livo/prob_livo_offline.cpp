#include "LIVMapper.h"
#include "prob_livo/offline_reader.h"

#include <filesystem>
#include <iostream>
#include <string>

#include <image_transport/image_transport.h>

namespace {

void ConfigureCameraOff(const std::string &output_directory,
                        const std::string &input_semantics) {
  ros::param::set("/common/img_en", 0);
  ros::param::set("/common/lidar_en", 1);
  ros::param::set("/common/prob_livo_backend", true);
  ros::param::set("/common/prob_livo_input_semantics", input_semantics);
  ros::param::set("/common/prob_livo_trajectory_path",
                  output_directory + "/trajectory.tum");
  ros::param::set("/imu/imu_en", true);
  ros::param::set("/evo/pose_output_en", false);
}

}  // namespace

int main(int argc, char **argv) {
  ros::init(argc, argv, "prob_livo_offline");
  if (argc < 3 || argc > 4) {
    std::cerr << "usage: prob_livo_offline BAG OUTPUT_DIR "
                 "[fast_native|super_ntu_legacy]\n";
    return 2;
  }

  const std::string bag_path = argv[1];
  const std::string output_directory = argv[2];
  const std::string input_semantics = argc == 4 ? argv[3] : "fast_native";
  std::error_code error;
  std::filesystem::create_directories(output_directory, error);
  if (error) {
    std::cerr << "cannot create output directory: " << error.message() << "\n";
    return 2;
  }
  ConfigureCameraOff(output_directory, input_semantics);

  ros::NodeHandle nh;
  image_transport::ImageTransport image_transport(nh);
  LIVMapper mapper(nh);
  mapper.initializeSubscribersAndPublishers(nh, image_transport);

  prob_livo::OfflineOptions options;
  options.bag_path = bag_path;
  options.lidar_topic = mapper.lid_topic;
  options.imu_topic = mapper.imu_topic;

  std::size_t process_invocations = 0;
  std::size_t successful_steps = 0;
  const auto step = [&]() {
    ++process_invocations;
    if (mapper.sync_packages(mapper.LidarMeasures)) {
      mapper.handleFirstFrame();
      mapper.processImu();
      mapper.stateEstimationAndMapping();
      ++successful_steps;
    }
  };

  prob_livo::OfflineReader reader;
  prob_livo::OfflineDispatch dispatch;
  dispatch.on_imu = [&](const sensor_msgs::Imu::ConstPtr &message) {
    mapper.imu_cbk(message);
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
  dispatch.step = step;

  if (!reader.run(options, dispatch)) return 1;

  // Match the bounded EOF drain used by the legacy offline production tool.
  for (int index = 0; index < 5 && ros::ok(); ++index) step();
  mapper.savePCD();

  const auto &accounting = reader.accounting();
  std::cout << "[prob_livo_offline] process_invocations="
            << process_invocations << " successful_steps=" << successful_steps
            << " lidar_read=" << accounting.lidar_read
            << " imu_read=" << accounting.imu_read
            << " speed_factor=" << accounting.speed_factor << "x\n";
  ros::shutdown();
  return 0;
}
