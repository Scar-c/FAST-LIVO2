#include "prob_livo/prob_imu_adapter.h"

#include <Eigen/Geometry>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace prob_livo {

namespace {

bool FiniteImu(const ImuSample &sample) {
  return std::isfinite(sample.timestamp) && sample.acceleration.allFinite() &&
         sample.angular_velocity.allFinite();
}

void SetFailure(ProbImuAdapter::Result &result, const std::string &message) {
  result.success = false;
  result.message = message;
}

}  // namespace

bool ProbImuAdapter::ResolveEpochTiming(const LidarMeasureGroup &measures,
                                        SchedulerMode mode,
                                        EpochTiming &timing,
                                        std::string &message) {
  if (!std::isfinite(measures.last_lio_update_time) ||
      measures.last_lio_update_time < 0.0) {
    message = "scheduler did not provide a finite last_lio_update_time";
    return false;
  }
  if (measures.measures.empty()) {
    message = "scheduler packet has no LIO measure group";
    return false;
  }
  if (!std::isfinite(measures.measures.back().lio_time)) {
    message = "scheduler packet has a non-finite lio_time";
    return false;
  }
  timing.epoch_start = measures.last_lio_update_time;
  timing.epoch_end = measures.measures.back().lio_time;
  // ONLY_LIO keeps the point curvature relative to the scan header.  LIVO
  // rebases both current and carried-over points to last_lio_update_time in
  // sync_packages, so those points use the LIO epoch origin.
  timing.point_time_origin =
      mode == SchedulerMode::kOnlyLio ? measures.lidar_frame_beg_time
                                      : timing.epoch_start;
  if (!std::isfinite(timing.epoch_start) ||
      !std::isfinite(timing.epoch_end) ||
      !std::isfinite(timing.point_time_origin) ||
      timing.epoch_end + 1e-12 < timing.epoch_start) {
    message = "invalid scheduler epoch timing";
    return false;
  }
  if (measures.pcl_proc_cur == nullptr) {
    message = "scheduler packet has no current point cloud";
    return false;
  }
  return true;
}

bool ProbImuAdapter::ToImuSample(const sensor_msgs::Imu::ConstPtr &message,
                                 ImuSample &sample,
                                 std::string &message_out) {
  if (message == nullptr) {
    message_out = "scheduler supplied a null IMU message";
    return false;
  }
  sample.timestamp = message->header.stamp.toSec();
  sample.acceleration = Eigen::Vector3d(
      message->linear_acceleration.x, message->linear_acceleration.y,
      message->linear_acceleration.z);
  sample.angular_velocity = Eigen::Vector3d(
      message->angular_velocity.x, message->angular_velocity.y,
      message->angular_velocity.z);
  if (!FiniteImu(sample)) {
    message_out = "scheduler supplied a non-finite IMU message";
    return false;
  }
  return true;
}

bool ProbImuAdapter::AccumulateInitialization(const MeasureGroup &measure,
                                              std::string &message) {
  for (const auto &imu_message : measure.imu) {
    ImuSample sample;
    if (!ToImuSample(imu_message, sample, message)) {
      return false;
    }
    const double count = static_cast<double>(initialization_samples_);
    mean_gyro_ = (mean_gyro_ * count + sample.angular_velocity) / (count + 1.0);
    mean_acceleration_ =
        (mean_acceleration_ * count + sample.acceleration) / (count + 1.0);
    ++initialization_samples_;
    last_initialization_imu_ = sample;
  }
  return true;
}

void ProbImuAdapter::InitializeFilter(ProbESKF19 &filter,
                                      const ImuSample &last_imu) {
  const double acceleration_norm = mean_acceleration_.norm();
  imu_scale_ = options_.gravity_norm / acceleration_norm;
  initial_gravity_ =
      -mean_acceleration_ * options_.gravity_norm / acceleration_norm;
  const Eigen::Vector3d reference_gravity(0.0, 0.0, -options_.gravity_norm);
  const Eigen::Quaterniond gravity_alignment =
      Eigen::Quaterniond::FromTwoVectors(initial_gravity_, reference_gravity);
  const Eigen::Matrix3d init_rotation =
      gravity_alignment.toRotationMatrix();
  const Eigen::Vector3d first_column = init_rotation.col(0);
  const double yaw = std::atan2(first_column.y(), first_column.x());
  const Eigen::Matrix3d yaw_inverse =
      Eigen::AngleAxisd(-yaw, Eigen::Vector3d::UnitZ()).toRotationMatrix();
  initial_rotation_ = options_.lidar_to_robot_yaw * yaw_inverse * init_rotation;

  filter.SetGravityNorm(options_.gravity_norm);
  filter.SetImuScale(imu_scale_);

  StatesGroup &state = filter.state();
  const Matrix19 previous_covariance = state.cov;
  Matrix18 physical_covariance = 1e-4 * Matrix18::Identity();
  physical_covariance.block<3, 3>(Layout::kRot0, Layout::kRot0) =
      (0.1 / 180.0 * M_PI) * Eigen::Matrix3d::Identity();
  Matrix19 initial_covariance =
      EmbedPhysicalCovariance(physical_covariance);
  // Exposure is a FAST visual state, not an IMU-initialized Super state.
  // Preserve its existing mean/covariance row and column exactly.
  for (int index = 0; index < Layout::kDim19; ++index) {
    initial_covariance(index, Layout::kExpo) =
        previous_covariance(index, Layout::kExpo);
    initial_covariance(Layout::kExpo, index) =
        previous_covariance(Layout::kExpo, index);
  }

  state.rot_end = initial_rotation_;
  state.pos_end = options_.robot_origin;
  state.vel_end.setZero();
  state.bias_g = mean_gyro_;
  state.bias_a.setZero();
  state.gravity = reference_gravity;
  state.cov = initial_covariance;

  filter.ResetImuHistory();
  filter.SeedImuHistory(last_imu);
  initialized_ = true;
}

ProbImuAdapter::Result ProbImuAdapter::ProcessLioEpoch(
    LidarMeasureGroup &measures, ProbESKF19 &filter, SchedulerMode mode,
    const ImuSample *lookahead_imu) {
  Result result;
  EpochTiming timing;
  if (!ResolveEpochTiming(measures, mode, timing, result.message)) {
    return result;
  }
  result.epoch_start = timing.epoch_start;
  result.epoch_end = timing.epoch_end;

  const MeasureGroup &measure = measures.measures.back();
  if (!initialized_) {
    if (!AccumulateInitialization(measure, result.message)) {
      return result;
    }
    if (initialization_samples_ >= options_.minimum_initialization_samples) {
      if (!(mean_acceleration_.norm() > std::numeric_limits<double>::epsilon())) {
        result.message = "cannot initialize from zero-norm mean acceleration";
        return result;
      }
      // The canonical wrapper timestamps the initialized state at the last
      // IMU in the accepted scheduler measure, including samples beyond the
      // minimum when a packet crosses the 50-sample transition.
      InitializeFilter(filter, last_initialization_imu_);
    }
    result.success = true;
    result.initialized = initialized_;
    return result;
  }

  if (!filter.has_imu_history()) {
    SetFailure(result, "initialized adapter has no ProbESKF19 IMU history");
    return result;
  }
  // Super's map-init consumes raw scans without propagating the ESKF.  At the
  // first RUN epoch the filter therefore legitimately still carries the
  // timestamp of the last IMU accepted during kf_init.  Reject only a filter
  // that is ahead of the scheduler epoch; Predict() uses the observation
  // window to bridge this initialization-to-runtime gap exactly once.
  if (filter.current_time() > timing.epoch_start +
                                  options_.time_tolerance) {
    SetFailure(result,
               "ProbESKF19 state is ahead of scheduler epoch start");
    return result;
  }

  std::vector<ImuSample> packet_imu;
  packet_imu.reserve(measure.imu.size());
  double previous_timestamp = -std::numeric_limits<double>::infinity();
  for (const auto &imu_message : measure.imu) {
    ImuSample sample;
    if (!ToImuSample(imu_message, sample, result.message)) return result;
    if (sample.timestamp + options_.time_tolerance < previous_timestamp) {
      SetFailure(result, "scheduler IMU packet is not time ordered");
      return result;
    }
    if (sample.timestamp > timing.epoch_end + options_.time_tolerance) {
      SetFailure(result, "scheduler consumed an IMU after the LIO endpoint");
      return result;
    }
    packet_imu.push_back(sample);
    previous_timestamp = sample.timestamp;
  }

  double latest_timestamp = filter.last_imu_time();
  for (const ImuSample &sample : packet_imu) {
    latest_timestamp = std::max(latest_timestamp, sample.timestamp);
  }
  const bool endpoint_already_covered =
      latest_timestamp + options_.time_tolerance >= timing.epoch_end;
  bool use_lookahead = false;
  if (!endpoint_already_covered) {
    if (options_.bridge_to_epoch_endpoint) {
      if (lookahead_imu == nullptr || !FiniteImu(*lookahead_imu) ||
          lookahead_imu->timestamp <= timing.epoch_end +
                                      options_.time_tolerance) {
        SetFailure(result,
                   "epoch endpoint requires a non-consuming IMU look-ahead");
        return result;
      }
      if (!packet_imu.empty() &&
          lookahead_imu->timestamp <= packet_imu.back().timestamp) {
        SetFailure(result, "IMU look-ahead is not after the scheduler packet");
        return result;
      }
      use_lookahead = true;
    }
  }

  for (const PointType &point : measures.pcl_proc_cur->points) {
    const double query_time = timing.point_time_origin +
                              static_cast<double>(point.curvature) / 1000.0;
    if (!std::isfinite(query_time)) {
      SetFailure(result, "scheduler point time is outside the LIO epoch");
      return result;
    }
  }

  filter.SetObservationWindow(timing.epoch_start, timing.epoch_end);
  propagation_trace_.clear();
  propagation_trace_.push_back(filter.CurrentPropagationSnapshot());
  for (const ImuSample &sample : packet_imu) {
    PropagationSnapshot snapshot;
    if (filter.Predict(sample, &snapshot)) {
      propagation_trace_.push_back(snapshot);
      ++result.propagated_samples;
    }
  }
  if (use_lookahead) {
    PropagationSnapshot snapshot;
    if (filter.Predict(*lookahead_imu, &snapshot)) {
      propagation_trace_.push_back(snapshot);
      ++result.propagated_samples;
    }
  }
  if (filter.current_time() > timing.epoch_end + options_.time_tolerance ||
      (options_.bridge_to_epoch_endpoint &&
       std::abs(filter.current_time() - timing.epoch_end) >
           options_.time_tolerance)) {
    SetFailure(result, "ProbESKF19 did not reach the requested LIO endpoint");
    return result;
  }

  if (!Undistort(measures, timing, filter, *result.prob_scan_undistort_imu,
                 result.message)) {
    result.success = false;
    return result;
  }

  // Scheduler anchor ownership belongs to ProbLioLifecycleAuthority.  The
  // adapter only returns a successful result; failed epochs never move it.
  result.success = true;
  result.initialized = true;
  result.propagated = true;
  return result;
}

bool ProbImuAdapter::Undistort(const LidarMeasureGroup &measures,
                               const EpochTiming &timing,
                               const ProbESKF19 &filter,
                               PointCloudXYZI &output,
                               std::string &message) const {
  if (propagation_trace_.empty()) {
    message = "cannot undistort without a propagation trace";
    return false;
  }
  const auto &trace = propagation_trace_;
  const double tolerance = options_.time_tolerance;
  const Eigen::Matrix3d end_rotation = filter.state().rot_end;
  const Eigen::Vector3d end_position = filter.state().pos_end;
  const Eigen::Matrix3d end_rotation_inverse = end_rotation.transpose();
  output.resize(measures.pcl_proc_cur->points.size());
  output.width = static_cast<std::uint32_t>(output.points.size());
  output.height = 1;
  output.is_dense = false;

  for (const PointType &input : measures.pcl_proc_cur->points) {
    const double query_time = timing.point_time_origin +
                              static_cast<double>(input.curvature) / 1000.0;
    if (!std::isfinite(query_time)) {
      message = "point query time is outside the propagated trace";
      return false;
    }
    if (query_time <= trace.front().timestamp + tolerance) {
      if (trace.size() < 2) {
        message = "propagated trace has no interval for an early point";
        return false;
      }
      if (!(trace[1].timestamp - trace.front().timestamp > tolerance)) {
        message = "propagated trace has an invalid first interval";
        return false;
      }
    }
  }

  tbb::parallel_for(
      tbb::blocked_range<std::size_t>(0,
                                      measures.pcl_proc_cur->points.size()),
      [&](const tbb::blocked_range<std::size_t> &range) {
        for (std::size_t index = range.begin(); index < range.end(); ++index) {
          const PointType &input = measures.pcl_proc_cur->points[index];
          PointType &undistorted = output.points[index];
          undistorted = input;
          const double query_time = timing.point_time_origin +
                                    static_cast<double>(input.curvature) /
                                        1000.0;

          // Super's Propagation_Undistort intentionally leaves points whose
          // source acquisition time is after the scan endpoint in the
          // LiDAR->IMU frame. This is required for the legacy Ouster
          // source-order/end-time contract.
          if (query_time > trace.back().timestamp) {
            const Eigen::Vector3d raw(input.x, input.y, input.z);
            const Eigen::Vector3d lidar_in_imu =
                options_.lidar_to_imu_rotation * raw +
                options_.lidar_to_imu_translation;
            undistorted.x = static_cast<float>(lidar_in_imu.x());
            undistorted.y = static_cast<float>(lidar_in_imu.y());
            undistorted.z = static_cast<float>(lidar_in_imu.z());
            continue;
          }

          PropagationSnapshot interpolated;
          if (query_time <= trace.front().timestamp + tolerance) {
            const PropagationSnapshot &head = trace.front();
            const PropagationSnapshot &tail = trace[1];
            const double dt = tail.timestamp - head.timestamp;
            const double tau = query_time - head.timestamp;
            const double ratio = tau / dt;
            interpolated.timestamp = query_time;
            const Eigen::Vector3d relative_log =
                LogSO3(head.rotation.transpose() * tail.rotation);
            interpolated.rotation = head.rotation * ExpSO3(relative_log * ratio);
            interpolated.position = head.position + head.velocity * tau +
                                    0.5 * tail.acceleration * tau * tau;
            interpolated.velocity = head.velocity;
            interpolated.acceleration = tail.acceleration;
            interpolated.angular_velocity = tail.angular_velocity;
          } else if (query_time >= trace.back().timestamp - tolerance) {
            interpolated = trace.back();
          } else {
            std::size_t upper = 1;
            while (upper < trace.size() &&
                   trace[upper].timestamp < query_time) {
              ++upper;
            }
            if (upper >= trace.size()) {
              interpolated = trace.back();
            } else {
              const PropagationSnapshot &head = trace[upper - 1];
              const PropagationSnapshot &tail = trace[upper];
              const double dt = tail.timestamp - head.timestamp;
              if (!(dt > tolerance)) {
                interpolated = tail;
              } else {
                const double tau = query_time - head.timestamp;
                const double ratio = tau / dt;
                interpolated.timestamp = query_time;
                interpolated.rotation =
                    Eigen::Quaterniond(head.rotation)
                        .slerp(ratio, Eigen::Quaterniond(tail.rotation))
                        .toRotationMatrix();
                interpolated.position =
                    head.position + head.velocity * tau +
                    0.5 * tail.acceleration * tau * tau;
                interpolated.velocity = head.velocity;
                interpolated.acceleration = tail.acceleration;
                interpolated.angular_velocity = tail.angular_velocity;
              }
            }
          }

          const Eigen::Vector3d raw(input.x, input.y, input.z);
          const Eigen::Vector3d lidar_in_imu =
              options_.lidar_to_imu_rotation * raw +
              options_.lidar_to_imu_translation;
          const Eigen::Vector3d end_frame_point =
              end_rotation_inverse *
              (interpolated.rotation * lidar_in_imu + interpolated.position -
               end_position);
          undistorted.x = static_cast<float>(end_frame_point.x());
          undistorted.y = static_cast<float>(end_frame_point.y());
          undistorted.z = static_cast<float>(end_frame_point.z());
        }
      });
  return true;
}

}  // namespace prob_livo
