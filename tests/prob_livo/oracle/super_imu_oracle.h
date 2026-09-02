#ifndef PROB_LIVO_TEST_SUPER_IMU_ORACLE_H_
#define PROB_LIVO_TEST_SUPER_IMU_ORACLE_H_

// Independent test oracle for Prompt 2.
// Provenance: Super-LIO reference repository
// /home/lc/super_livo/ref/Super-LIO, commit
// 9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e,
// src/super_lio/src/lio/super_lio.cpp (kf_init lines 120-165 and
// Propagation_Undistort lines 384-449),
// src/super_lio/src/lio/ESKF.cpp (Predict lines 195-249), and
// src/super_lio/src/lio/ESKF.cpp SetInitialConditions.
// This file deliberately does not call ProbImuAdapter or ProbESKF19.

#include "prob_livo/prob_eskf19.h"
#include "oracle/super_eskf_oracle.h"

#include <Eigen/Geometry>

#include <cmath>
#include <vector>

namespace prob_livo_test_oracle {

struct InitConfig {
  int minimum_samples = 50;
  double gravity_norm = 9.7946;
  Eigen::Matrix3d lidar_to_robot_yaw = Eigen::Matrix3d::Identity();
  Eigen::Vector3d robot_origin = Eigen::Vector3d::Zero();
};

struct InitResult {
  int samples = 0;
  V3 mean_gyro = V3::Zero();
  V3 mean_acceleration = V3::Zero();
  double imu_scale = 1.0;
  V3 gravity = V3::Zero();
  M3 rotation = M3::Identity();
  V3 position = V3::Zero();
  double timestamp = 0.0;
  M18 covariance = M18::Identity();
};

inline InitResult Initialize(const std::vector<prob_livo::ImuSample> &samples,
                             const InitConfig &config) {
  InitResult result;
  for (const auto &sample : samples) {
    const double count = static_cast<double>(result.samples);
    result.mean_gyro =
        (result.mean_gyro * count + sample.angular_velocity) / (count + 1.0);
    result.mean_acceleration =
        (result.mean_acceleration * count + sample.acceleration) /
        (count + 1.0);
    ++result.samples;
    result.timestamp = sample.timestamp;
    if (result.samples >= config.minimum_samples) break;
  }
  result.imu_scale = config.gravity_norm / result.mean_acceleration.norm();
  result.gravity = -result.mean_acceleration * config.gravity_norm /
                   result.mean_acceleration.norm();
  const V3 reference_gravity(0.0, 0.0, -config.gravity_norm);
  const Eigen::Quaterniond alignment =
      Eigen::Quaterniond::FromTwoVectors(result.gravity, reference_gravity);
  const M3 gravity_rotation = alignment.toRotationMatrix();
  const V3 first_column = gravity_rotation.col(0);
  const double yaw = std::atan2(first_column.y(), first_column.x());
  const M3 yaw_inverse =
      Eigen::AngleAxisd(-yaw, V3::UnitZ()).toRotationMatrix();
  result.rotation = config.lidar_to_robot_yaw * yaw_inverse * gravity_rotation;
  result.position = config.robot_origin;
  result.covariance = 1e-4 * M18::Identity();
  result.covariance.block<3, 3>(0, 0) =
      (0.1 / 180.0 * M_PI) * M3::Identity();
  return result;
}

struct DynamicSnapshot {
  double timestamp = 0.0;
  M3 rotation = M3::Identity();
  V3 position = V3::Zero();
  V3 velocity = V3::Zero();
  V3 acceleration = V3::Zero();
  V3 angular_velocity = V3::Zero();
};

class Propagation {
 public:
  explicit Propagation(const Options &options) : options_(options) {}

  State state;
  double current_time = 0.0;
  double last_imu_time = -1.0;
  double last_observation_time = 0.0;
  double current_observation_time = 0.0;
  V3 last_acceleration = V3::Zero();
  V3 last_angular_velocity = V3::Zero();

  void Seed(const prob_livo::ImuSample &imu) {
    last_imu_ = imu;
    last_imu_time = imu.timestamp;
    current_time = imu.timestamp;
    has_last_imu_ = true;
  }

  DynamicSnapshot Snapshot() const {
    return DynamicSnapshot{current_time, state.rotation, state.position,
                           state.velocity, last_acceleration,
                           last_angular_velocity};
  }

  bool Predict(const prob_livo::ImuSample &imu) {
    if (!has_last_imu_) {
      Seed(imu);
      return false;
    }
    if (imu.timestamp <= last_observation_time) {
      last_imu_ = imu;
      last_imu_time = imu.timestamp;
      return false;
    }
    current_time = imu.timestamp;
    double dt = imu.timestamp - last_imu_time;
    if (last_imu_time < last_observation_time) {
      dt = imu.timestamp - last_observation_time;
    } else if (imu.timestamp > current_observation_time) {
      dt = current_observation_time - last_imu_time;
      current_time = current_observation_time;
    }
    const V3 acceleration =
        options_.imu_scale * 0.5 *
            (imu.acceleration + last_imu_.acceleration) - state.accel_bias;
    const V3 angular_velocity =
        0.5 * (imu.angular_velocity + last_imu_.angular_velocity) -
        state.gyro_bias;
    Predict(state, prob_livo::ImuSample{imu.timestamp, imu.acceleration,
                                        imu.angular_velocity},
            prob_livo::ImuSample{last_imu_.timestamp,
                                 last_imu_.acceleration,
                                 last_imu_.angular_velocity},
            dt);
    // The source computes global_acc before changing R.  Reconstruct it from
    // the pre-update rotation saved above, matching ESKF::Predict exactly.
    last_acceleration = previous_rotation_ * acceleration + state.gravity;
    last_angular_velocity = angular_velocity;
    last_imu_ = imu;
    last_imu_time = imu.timestamp;
    return true;
  }

 private:
  void Predict(State &state, const prob_livo::ImuSample &current,
               const prob_livo::ImuSample &previous, double dt) {
    const V3 acceleration =
        options_.imu_scale * 0.5 *
            (current.acceleration + previous.acceleration) - state.accel_bias;
    const V3 angular_velocity =
        0.5 * (current.angular_velocity + previous.angular_velocity) -
        state.gyro_bias;
    previous_rotation_ = state.rotation;
    prob_livo_test_oracle::Predict(
        state, prob_livo_test_oracle::ImuPair{
                   previous.acceleration, current.acceleration,
                   previous.angular_velocity, current.angular_velocity, dt},
        options_);
  }

  Options options_;
  bool has_last_imu_ = false;
  prob_livo::ImuSample last_imu_;
  M3 previous_rotation_ = M3::Identity();
};

inline V3 UndistortPoint(
    const V3 &raw_lidar, double query_time,
    const std::vector<DynamicSnapshot> &trace,
    const M3 &lidar_to_imu_rotation, const V3 &lidar_to_imu_translation) {
  std::size_t upper = 1;
  while (upper < trace.size() && trace[upper].timestamp < query_time) ++upper;
  DynamicSnapshot interpolated;
  if (query_time <= trace.front().timestamp) {
    interpolated = trace.front();
  } else if (query_time >= trace.back().timestamp || upper >= trace.size()) {
    interpolated = trace.back();
  } else {
    const DynamicSnapshot &head = trace[upper - 1];
    const DynamicSnapshot &tail = trace[upper];
    const double dt = tail.timestamp - head.timestamp;
    const double tau = query_time - head.timestamp;
    const double ratio = tau / dt;
    interpolated.timestamp = query_time;
    interpolated.rotation =
        Eigen::Quaterniond(head.rotation)
            .slerp(ratio, Eigen::Quaterniond(tail.rotation))
            .toRotationMatrix();
    interpolated.position =
        head.position + head.velocity * tau + 0.5 * tail.acceleration * tau * tau;
  }
  const M3 end_rotation = trace.back().rotation;
  const V3 end_position = trace.back().position;
  const V3 lidar_in_imu = lidar_to_imu_rotation * raw_lidar +
                          lidar_to_imu_translation;
  return end_rotation.transpose() *
         (interpolated.rotation * lidar_in_imu + interpolated.position -
          end_position);
}

}  // namespace prob_livo_test_oracle

#endif  // PROB_LIVO_TEST_SUPER_IMU_ORACLE_H_
