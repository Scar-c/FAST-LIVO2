#include "LIVMapper.h"
#include "prob_livo/benchmark_runtime.h"
#include "prob_livo/offline_reader.h"

#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <chrono>

#include <image_transport/image_transport.h>
#include <omp.h>
#include <tbb/global_control.h>

namespace {

std::string Environment(const char *name, const std::string &fallback) {
  const char *value = std::getenv(name);
  return value == nullptr || *value == '\0' ? fallback : value;
}

bool IsCameraMode(const std::string &mode) {
  return mode == "off" || mode == "h0" || mode == "h1" || mode == "h2";
}

int PositiveEnvironment(const char *name, int fallback) {
  try {
    const int value = std::stoi(Environment(name, std::to_string(fallback)));
    return value > 0 ? value : fallback;
  } catch (...) {
    return fallback;
  }
}

void ConfigureRuntime(const std::string &output_directory,
                      const std::string &input_semantics,
                      const std::string &camera_mode,
                      const std::string &visual_gate) {
  const bool camera_on = camera_mode != "off";
  const bool visual_on = camera_mode == "h1" || camera_mode == "h2";
  ros::param::set("/common/img_en", camera_on ? 1 : 0);
  ros::param::set("/common/lidar_en", 1);
  ros::param::set("/common/prob_livo_backend", true);
  ros::param::set("/common/prob_livo_camera_vio", visual_on);
  ros::param::set("/common/prob_livo_input_semantics", input_semantics);
  ros::param::set("/prob_livo/visual_plane_gate", visual_gate);
  ros::param::set("/common/prob_livo_trajectory_path",
                  output_directory + "/trajectory.tum");
  ros::param::set("/imu/imu_en", true);
  ros::param::set("/evo/pose_output_en", false);
  ros::param::set("/evo/runtime_report_directory", output_directory);
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
  const std::string camera_mode = Environment("PROB_LIVO_CAMERA_MODE", "off");
  const std::string visual_gate = Environment(
      "PROB_LIVO_VISUAL_PLANE_GATE", "livo2_prob_3sigma");
  if (!IsCameraMode(camera_mode) ||
      (visual_gate != "livo2_prob_3sigma" && visual_gate != "super_legacy")) {
    std::cerr << "invalid PROB_LIVO_CAMERA_MODE or "
                 "PROB_LIVO_VISUAL_PLANE_GATE\n";
    return 2;
  }
  const int worker_limit = PositiveEnvironment("PROB_LIVO_WORKERS", 32);
  tbb::global_control tbb_control(
      tbb::global_control::max_allowed_parallelism, worker_limit);
  omp_set_num_threads(worker_limit);
  std::error_code error;
  std::filesystem::create_directories(output_directory, error);
  if (error) {
    std::cerr << "cannot create output directory: " << error.message() << "\n";
    return 2;
  }
  ConfigureRuntime(output_directory, input_semantics, camera_mode,
                   visual_gate);

  ros::NodeHandle nh;
  image_transport::ImageTransport image_transport(nh);
  LIVMapper mapper(nh);
  mapper.initializeSubscribersAndPublishers(nh, image_transport);
  prob_livo::BenchmarkProcessMonitor monitor;
  monitor.Start(output_directory + "/memory.csv", "prob", 2.0);
  const auto offline_wall_begin = std::chrono::steady_clock::now();
  const double offline_cpu_begin = prob_livo::ProcessCpuSeconds();

  prob_livo::OfflineOptions options;
  options.bag_path = bag_path;
  options.lidar_topic = mapper.lid_topic;
  options.imu_topic = mapper.imu_topic;
  options.image_topic = mapper.img_en ? mapper.img_topic : "";
  options.sensor_progress = [&](double timestamp) {
    monitor.SetSensorTimestamp(timestamp);
  };

  std::size_t process_invocations = 0;
  std::size_t successful_steps = 0;
  const auto step = [&]() {
    ++process_invocations;
    if (mapper.ProcessAvailableBenchmarkEpoch()) ++successful_steps;
  };

  prob_livo::OfflineReader reader;
  prob_livo::OfflineDispatch dispatch;
  dispatch.on_imu = [&](const sensor_msgs::Imu::ConstPtr &message) {
    mapper.imu_cbk(message);
  };
  if (mapper.img_en) {
    dispatch.on_image = [&](const sensor_msgs::ImageConstPtr &message) {
      mapper.img_cbk(message);
    };
  }
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

  const std::size_t drain_steps = mapper.DrainAvailableBenchmarkEpochs();
  mapper.DrainUnprocessableInputBuffers();
  mapper.savePCD();

  mapper.writeBenchmarkReports(output_directory);
  const auto &runtime = mapper.benchmark_runtime_counters();
  const auto &timing = mapper.benchmark_runtime_timing();
  const auto &accounting = reader.accounting();
  const bool callbacks_drained =
      accounting.imu_read == runtime.imu_callbacks_received &&
      accounting.lidar_read == runtime.lidar_callbacks_received &&
      (camera_mode == "off" ||
       accounting.image_read == runtime.image_callbacks_received);
  const bool expected_final_epoch_reached =
      callbacks_drained && runtime.scheduler_step_calls > 0 &&
      runtime.scheduler_step_calls == runtime.scheduler_sync_packages;
  mapper.writeProcessingCompleteSentinel(
      output_directory, "offline_rosbag_record_order", true,
      expected_final_epoch_reached);
  const double offline_cpu_s =
      prob_livo::ProcessCpuSeconds() - offline_cpu_begin;
  const double offline_wall_s = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - offline_wall_begin).count();
  monitor.Stop();

  std::ofstream source(output_directory + "/offline_source.yaml");
  source << "schema_version: 2\n"
         << "bag_relevant_messages: " << accounting.bag_relevant_messages
         << "\n"
         << "lidar_read: " << accounting.lidar_read << "\n"
         << "imu_read: " << accounting.imu_read << "\n"
         << "image_read: " << accounting.image_read << "\n"
         << "other_messages: " << accounting.other_messages << "\n"
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
         << "worker_limit: " << worker_limit << "\n";

  std::cout << "[prob_livo_offline] process_invocations="
            << process_invocations << " successful_steps=" << successful_steps
            << " lidar_read=" << accounting.lidar_read
            << " imu_read=" << accounting.imu_read
            << " image_read=" << accounting.image_read
            << " camera_mode=" << camera_mode
            << " visual_plane_gate=" << visual_gate
            << " tbb_max_parallelism=" << worker_limit
            << " omp_max_threads=" << omp_get_max_threads()
            << " speed_factor=" << accounting.speed_factor << "x\n";
  ros::shutdown();
  return 0;
}
