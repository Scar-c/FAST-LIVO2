#ifndef PROB_LIVO_PROB_IMU_ADAPTER_H_
#define PROB_LIVO_PROB_IMU_ADAPTER_H_

#include "prob_livo/prob_eskf19.h"
#include "prob_livo/prob_lio_lifecycle.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace prob_livo {

enum class ImuInitializationSemantics {
  kSuperLegacy,
  kFastLivo2Native,
};

enum class SchedulerMode { kOnlyLio, kLivo };

struct EpochTiming {
  double epoch_start = 0.0;
  double epoch_end = 0.0;
  double point_time_origin = 0.0;
};

/**
 * The exact current/next point rebase used by FAST's LIVO scheduler.
 * Production sync_packages calls this helper; tests use the same seam to
 * verify the scheduler contract without fabricating a final packet.
 */
enum class LivoPointBucket { kCurrent, kNext };

inline LivoPointBucket RebaseLivoPoint(const PointType &source,
                                       double frame_header_time,
                                       double previous_lio_time,
                                       double current_lio_time,
                                       PointType &rebased) {
  const float max_offset_ms =
      static_cast<float>((current_lio_time - frame_header_time) * 1000.0);
  rebased = source;
  if (source.curvature < max_offset_ms) {
    rebased.curvature += static_cast<float>(
        (frame_header_time - previous_lio_time) * 1000.0);
    return LivoPointBucket::kCurrent;
  }
  rebased.curvature += static_cast<float>(
      (frame_header_time - current_lio_time) * 1000.0);
  return LivoPointBucket::kNext;
}

class ProbImuAdapter {
 public:
  struct Options {
    int minimum_initialization_samples = 50;
    double gravity_norm = 9.7946;
    ImuInitializationSemantics initialization_semantics =
        ImuInitializationSemantics::kSuperLegacy;
    Eigen::Matrix3d lidar_to_imu_rotation = Eigen::Matrix3d::Identity();
    Eigen::Vector3d lidar_to_imu_translation = Eigen::Vector3d::Zero();
    Eigen::Matrix3d lidar_to_robot_yaw = Eigen::Matrix3d::Identity();
    Eigen::Vector3d robot_origin = Eigen::Vector3d::Zero();
    // FAST-native scheduling bridges the state to the scan endpoint with a
    // non-consuming IMU look-ahead.  Legacy Super leaves the state at the
    // last consumed IMU and undistorts later points in the raw sensor frame.
    bool bridge_to_epoch_endpoint = true;
    double time_tolerance = 1e-9;
  };

  struct Result {
    bool success = false;
    bool initialized = false;
    bool propagated = false;
    double epoch_start = 0.0;
    double epoch_end = 0.0;
    std::size_t propagated_samples = 0;
    PointCloudXYZI::Ptr prob_scan_undistort_imu;
    std::string message;

    Result() : prob_scan_undistort_imu(new PointCloudXYZI()) {}
  };

  ProbImuAdapter() = default;
  explicit ProbImuAdapter(const Options &options) : options_(options) {}

  Result ProcessLioEpoch(LidarMeasureGroup &measures, ProbESKF19 &filter,
                         SchedulerMode mode,
                         const ImuSample *lookahead_imu = nullptr);

  bool initialized() const { return initialized_; }
  int initialization_samples() const { return initialization_samples_; }
  const Eigen::Vector3d &mean_gyro() const { return mean_gyro_; }
  const Eigen::Vector3d &mean_acceleration() const { return mean_acceleration_; }
  double initialization_window_start() const {
    return initialization_window_start_;
  }
  double initialization_window_end() const { return initialization_window_end_; }
  double imu_scale() const { return imu_scale_; }
  const Eigen::Vector3d &initial_gravity() const { return initial_gravity_; }
  const Eigen::Matrix3d &initial_rotation() const { return initial_rotation_; }
  const std::vector<PropagationSnapshot> &propagation_trace() const {
    return propagation_trace_;
  }

  static bool ResolveEpochTiming(const LidarMeasureGroup &measures,
                                 SchedulerMode mode, EpochTiming &timing,
                                 std::string &message);

 private:
  static bool ToImuSample(const sensor_msgs::Imu::ConstPtr &message,
                          ImuSample &sample, std::string &message_out);
  bool AccumulateInitialization(const MeasureGroup &measure,
                                std::string &message);
  void InitializeFilter(ProbESKF19 &filter, const ImuSample &last_imu);
  void InitializeFastLivo2Native(ProbESKF19 &filter,
                                const ImuSample &last_imu);
  bool Undistort(const LidarMeasureGroup &measures, const EpochTiming &timing,
                const ProbESKF19 &filter, PointCloudXYZI &output,
                std::string &message) const;

  Options options_;
  bool initialized_ = false;
  int initialization_samples_ = 0;
  double initialization_window_start_ =
      std::numeric_limits<double>::quiet_NaN();
  double initialization_window_end_ =
      std::numeric_limits<double>::quiet_NaN();
  Eigen::Vector3d gyro_sum_ = Eigen::Vector3d::Zero();
  Eigen::Vector3d acceleration_sum_ = Eigen::Vector3d::Zero();
  Eigen::Vector3d mean_gyro_ = Eigen::Vector3d::Zero();
  Eigen::Vector3d mean_acceleration_ = Eigen::Vector3d::Zero();
  ImuSample last_initialization_imu_;
  double imu_scale_ = 1.0;
  Eigen::Vector3d initial_gravity_ = Eigen::Vector3d::Zero();
  Eigen::Matrix3d initial_rotation_ = Eigen::Matrix3d::Identity();
  std::vector<PropagationSnapshot> propagation_trace_;
};

}  // namespace prob_livo

#endif  // PROB_LIVO_PROB_IMU_ADAPTER_H_
