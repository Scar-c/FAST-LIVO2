#ifndef PROB_LIVO_PROB_LIO_BACKEND_H_
#define PROB_LIVO_PROB_LIO_BACKEND_H_

#include "prob_livo/prob_imu_adapter.h"
#include "prob_livo/prob_lio_lifecycle.h"
#include "prob_livo/prob_plane_provider.h"
#include "prob_livo/prob_point_with_var_adapter.h"
#include "prob_livo/super_native/OctVoxMap/OctVoxMap.hpp"
#include "prob_livo/super_native/OctVoxMap/VoxelGridFilter.h"
#include "prob_livo/super_native/prob_geometry_p0_p4.h"
#include "prob_livo/super_native/prob_qr_plane.h"

#include <fstream>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace prob_livo {

// Camera-OFF Prob-LIO backend.  This class is the sole owner of lifecycle,
// per-scan buffers, map state, and the shared ProbESKF19 reference.  FAST's
// LIVMapper remains the ROS/scheduler shell.
class ProbLioBackend {
 public:
  struct Options {
    int minimum_initialization_samples = 50;
    int map_initialization_scans = 4;
    int max_iterations = 4;
    double quit_eps = 0.001;
    double gravity_norm = 9.7946;
    double imu_gyro_variance = 0.1;
    double imu_accelerometer_variance = 0.1;
    double imu_gyro_bias_variance = 0.0001;
    double imu_accelerometer_bias_variance = 0.0001;
    double map_resolution = 0.5;
    std::size_t map_capacity = 100000000;
    double voxel_size = 0.5;
    double lidar_depth_error = 0.02;
    double lidar_beam_error_deg = 0.01;
    double map_covariance_validation_tolerance = 1e-9;
    LI2Sup::MapPoseCovModel map_pose_cov_model =
        LI2Sup::MapPoseCovModel::Livo2Compat;
    LI2Sup::CovStoragePrecision cov_storage_precision =
        LI2Sup::CovStoragePrecision::Double;
    bool covariance_pipeline_enabled = true;
    bool qr_plane_covariance_enabled = true;
    LI2Sup::P2pWeightMode p2p_weight_mode = LI2Sup::P2pWeightMode::ProbLivo2;
    LI2Sup::CovValidationMode covariance_validation_mode =
        LI2Sup::CovValidationMode::Light;
    bool legacy_super_timing = false;
    Eigen::Matrix3d lidar_to_imu_rotation = Eigen::Matrix3d::Identity();
    Eigen::Vector3d lidar_to_imu_translation = Eigen::Vector3d::Zero();
    Eigen::Matrix3d lidar_to_robot_yaw = Eigen::Matrix3d::Identity();
    Eigen::Vector3d robot_origin = Eigen::Vector3d::Zero();
    std::string trajectory_path;
  };

  struct Counters {
    std::size_t successful_epochs = 0;
    std::size_t imu_init_epochs = 0;
    std::size_t map_init_epochs = 0;
    std::size_t run_epochs = 0;
    std::size_t map_init_inserts = 0;
    std::size_t map_update_inserts = 0;
    std::size_t undistorted_points = 0;
    std::size_t downsampled_points = 0;
    std::size_t hknn_queries = 0;
    std::size_t hknn_returns = 0;
    std::size_t qr_attempted = 0;
    std::size_t qr_valid = 0;
    std::size_t weighted_measurements = 0;
    std::size_t legacy_measurements = 0;
    std::size_t lidar_callbacks_received = 0;
    std::size_t scheduler_epochs_emitted = 0;
    std::size_t scheduler_lidar_pending_at_shutdown = 0;
    std::size_t scheduler_lidar_discarded = 0;
    std::size_t backend_epochs_attempted = 0;
    std::size_t backend_epochs_success = 0;
    std::size_t backend_epochs_rejected = 0;
    std::size_t trajectory_rows = 0;
    std::size_t adapted_scans = 0;
    std::size_t adapted_points = 0;
  };

  ProbLioBackend(StatesGroup &state, const Options &options);
  ~ProbLioBackend();

  bool ProcessEpoch(LidarMeasureGroup &measures);
  const ProbLioLifecycleAuthority &lifecycle() const { return lifecycle_; }
  ProbLioLifecycle lifecycle_state() const { return lifecycle_.lifecycle(); }
  const Counters &counters() const { return counters_; }
  const std::string &last_error() const { return last_error_; }
  const PointCloudXYZI::Ptr &undistorted_scan() const { return undistorted_scan_; }
  const PointCloudXYZI::Ptr &world_scan() const { return world_scan_; }
  const std::vector<pointWithVar> &current_scan_point_with_var() const {
    return point_with_var_adapter_.output().points;
  }
  const std::vector<ProbPointIdentity> &current_scan_point_identities() const {
    return point_with_var_adapter_.output().identities;
  }
  const ProbPlaneProvider &plane_provider() const { return plane_provider_; }
  StatesGroup &state() { return state_; }
  const StatesGroup &state() const { return state_; }

  // Runtime accounting hooks used by the FAST ROS shell. They record event
  // ownership without creating a second scheduler or lifecycle authority.
  void RecordLidarCallback() { ++counters_.lidar_callbacks_received; }
  void RecordSchedulerEpochEmitted() { ++counters_.scheduler_epochs_emitted; }
  void RecordSchedulerPendingLidar(std::size_t count) {
    counters_.scheduler_lidar_pending_at_shutdown = count;
  }
  void RecordSchedulerDiscardedLidar(std::size_t count) {
    counters_.scheduler_lidar_discarded += count;
  }
  double last_observation_time() const { return filter_.last_observation_time(); }
  double filter_current_time() const { return filter_.current_time(); }

 private:
  bool ProcessImuInit(LidarMeasureGroup &measures, double epoch_end);
  bool ProcessMapInit(LidarMeasureGroup &measures, double epoch_end);
  bool ProcessRun(LidarMeasureGroup &measures, double epoch_end);
  bool ConvertLookahead(const sensor_msgs::Imu::ConstPtr &message,
                        ImuSample &sample) const;
  bool InsertInitialMap(const PointCloudXYZI &raw_scan);
  bool BuildAndSolveScan();
  void UpdateMap();
  void BuildWorldScan();
  void AppendTrajectory(double timestamp);
  void SetError(const std::string &message) { last_error_ = message; }

  struct ObservationContribution {
    Matrix6 hth = Matrix6::Zero();
    Vector6 htr = Vector6::Zero();
    std::size_t hknn_returns = 0;
    bool hknn_queried = false;
    bool qr_attempted = false;
    bool qr_valid = false;
    bool weighted = false;
    bool legacy = false;
  };

  StatesGroup &state_;
  Options options_;
  ProbESKF19 filter_;
  ProbImuAdapter imu_adapter_;
  ProbPointWithVarAdapter point_with_var_adapter_;
  ProbLioLifecycleAuthority lifecycle_;
  std::shared_ptr<LI2Sup::OctVoxMap<LI2Sup::V3, LI2Sup::scalar>> map_;
  ProbPlaneProvider plane_provider_;
  LI2Sup::VoxelGridClosest<PointType> downsample_filter_;

  bool anchor_seeded_ = false;
  int map_init_scan_count_ = 0;
  Counters counters_;
  std::string last_error_;
  std::ofstream trajectory_;

  PointCloudXYZI::Ptr undistorted_scan_{new PointCloudXYZI()};
  PointCloudXYZI::Ptr downsampled_scan_{new PointCloudXYZI()};
  PointCloudXYZI::Ptr world_scan_{new PointCloudXYZI()};

  LI2Sup::VV3 points_body_;
  std::vector<LI2Sup::M3d> lidar_covariances_;
  std::vector<LI2Sup::M3d> body_covariances_;
  std::vector<LI2Sup::M3d> map_covariances_;
  std::vector<LI2Sup::ProbQrPlane> plane_covariances_;
  std::vector<Eigen::Vector4d> plane_coefficients_;
  std::vector<std::uint8_t> effect_mask_;
};

}  // namespace prob_livo

#endif  // PROB_LIVO_PROB_LIO_BACKEND_H_
