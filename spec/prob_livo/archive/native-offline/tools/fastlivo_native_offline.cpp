#include "LIVMapper.h"
#include "native_benchmark_monitor.h"
#include "native_offline_reader.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <iomanip>

#include <image_transport/image_transport.h>

namespace {

bool IsMode(const std::string &mode)
{
  return mode == "lio" || mode == "livo";
}

std::size_t CameraStride()
{
  const char *value = std::getenv("FAST_LIVO_CAMERA_STRIDE");
  if (value == nullptr || *value == '\0') return 1;
  try {
    const long long parsed = std::stoll(value);
    return parsed > 0 ? static_cast<std::size_t>(parsed) : 1;
  } catch (...) {
    return 1;
  }
}

std::string VisualMemoryStage()
{
  const char *value = std::getenv("FAST_LIVO_VISUAL_MEMORY_STAGE");
  if (value == nullptr || *value == '\0') return "current";
  return value;
}

bool CrossModalFEJEnabled()
{
  const char *value = std::getenv("FAST_LIVO_CROSS_MODAL_FEJ");
  if (value == nullptr || *value == '\0') return false;
  return std::string(value) == "1" || std::string(value) == "true" ||
         std::string(value) == "TRUE" || std::string(value) == "on";
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
  const std::size_t camera_stride = CameraStride();
  const std::string visual_memory_stage = VisualMemoryStage();
  if (visual_memory_stage != "current" && visual_memory_stage != "leak_fix") {
    std::cerr << "invalid FAST_LIVO_VISUAL_MEMORY_STAGE\n";
    return 2;
  }
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
  ros::param::set("/common/prob_livo_visual_memory_stage",
                  visual_memory_stage);
  ros::param::set("/common/prob_livo_cross_modal_fej",
                  CrossModalFEJEnabled());
  ros::param::set("/common/cross_modal_fej_diagnostics_path",
                  output_directory + "/fej_diagnostics.csv");

  ros::NodeHandle nh;
  image_transport::ImageTransport image_transport(nh);
  LIVMapper mapper(nh);
  mapper.initializeSubscribersAndPublishers(nh, image_transport);
  fast_livo::NativeBenchmarkProcessMonitor monitor;
  monitor.Start(output_directory + "/memory.csv", "native", 2.0);
  const auto offline_wall_begin = std::chrono::steady_clock::now();
  const double offline_cpu_begin = fast_livo::ProcessCpuSeconds();

  fast_livo::NativeOfflineOptions options;
  options.bag_path = bag_path;
  options.lidar_topic = mapper.lid_topic;
  options.imu_topic = mapper.imu_topic;
  options.image_topic = mapper.img_en ? mapper.img_topic : "";
  options.image_stride = camera_stride;
  options.sensor_progress = [&](double timestamp) {
    monitor.SetSensorTimestamp(timestamp);
  };

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
  mapper.DrainUnprocessableInputBuffers();
  mapper.savePCD();
  mapper.writeRuntimeReports(output_directory);

  const auto &runtime = mapper.runtime_counters();
  const auto &timing = mapper.runtime_timing();
  const auto &accounting = reader.accounting();
  std::ofstream selected_images(output_directory +
                                "/selected_camera_timestamps.txt");
  selected_images << std::setprecision(17);
  for (const double timestamp : accounting.selected_image_timestamps)
    selected_images << timestamp << "\n";
  const bool callbacks_drained =
      accounting.imu_read == runtime.imu_callbacks_received &&
      accounting.lidar_read == runtime.lidar_callbacks_received &&
      (mode == "lio" ||
       accounting.image_read == runtime.image_callbacks_received);
  const bool expected_final_epoch_reached =
      callbacks_drained && runtime.trajectory_rows > 0 &&
      runtime.scheduler_step_calls == runtime.scheduler_sync_packages;
  mapper.writeProcessingCompleteSentinel(output_directory,
                                         "offline_rosbag_record_order", true,
                                         expected_final_epoch_reached);
  const double offline_cpu_s =
      fast_livo::ProcessCpuSeconds() - offline_cpu_begin;
  const double offline_wall_s = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - offline_wall_begin).count();
  monitor.Stop();

  std::ofstream source(output_directory + "/offline_source.yaml");
  source << "schema_version: 2\n"
         << "bag_relevant_messages: " << accounting.bag_relevant_messages << "\n"
         << "lidar_read: " << accounting.lidar_read << "\n"
         << "imu_read: " << accounting.imu_read << "\n"
         << "image_read: " << accounting.image_read << "\n"
         << "image_seen: " << accounting.image_seen << "\n"
         << "image_dropped: " << accounting.image_dropped << "\n"
         << "image_stride: " << camera_stride << "\n"
         << "visual_memory_stage: " << visual_memory_stage << "\n"
         << "ignored_records: " << accounting.ignored_records << "\n"
         << "first_bag_time: " << accounting.first_bag_time << "\n"
         << "last_bag_time: " << accounting.last_bag_time << "\n"
         << "first_sensor_time: " << accounting.first_sensor_time << "\n"
         << "last_sensor_time: " << accounting.last_sensor_time << "\n"
         << "reader_total_wall_s: " << accounting.wall_processing_s << "\n"
         << "image_decode_s: " << accounting.image_decode_s << "\n"
         << "image_decode_failures: " << accounting.image_decode_failures
         << "\n"
         << "io_decode_dispatch_residual_s: "
         << std::max(0.0, accounting.wall_processing_s -
                              timing.estimator_compute_s -
                              timing.input_preprocess_s)
         << "\n"
         << "eof_drain_steps: " << drain_steps << "\n";
  std::ofstream system(output_directory + "/offline_system.yaml");
  system << "schema_version: 1\n"
         << "offline_wall_s: " << offline_wall_s << "\n"
         << "offline_process_cpu_s: " << offline_cpu_s << "\n"
         << "worker_limit: " << MP_PROC_NUM << "\n";

  std::cout << "[fastlivo_native_offline] mode=" << mode
            << " lidar_read=" << accounting.lidar_read
            << " imu_read=" << accounting.imu_read
            << " image_read=" << accounting.image_read
            << " eof_drain_steps=" << drain_steps
            << " speed_factor=" << accounting.speed_factor << "x\n";
  ros::shutdown();
  return 0;
}
