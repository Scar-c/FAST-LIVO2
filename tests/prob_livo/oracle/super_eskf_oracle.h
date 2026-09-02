#ifndef PROB_LIVO_TEST_SUPER_ESKF_ORACLE_H_
#define PROB_LIVO_TEST_SUPER_ESKF_ORACLE_H_

// Test-only independent oracle.
// Provenance: Super-LIO reference repository
// /home/lc/super_livo/ref/Super-LIO, commit
// 9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e,
// src/super_lio/src/lio/ESKF.cpp (Predict lines 187-249 and UpdateObserve
// lines 251-336), with state declarations from ESKF.h lines 12-123.
// This deliberately does not include or link the production ProbESKF19 code.

#include <Eigen/Dense>

#include <cmath>
#include <functional>

namespace prob_livo_test_oracle {

using V3 = Eigen::Vector3d;
using V6 = Eigen::Matrix<double, 6, 1>;
using V18 = Eigen::Matrix<double, 18, 1>;
using M3 = Eigen::Matrix3d;
using M6 = Eigen::Matrix<double, 6, 6>;
using M12 = Eigen::Matrix<double, 12, 12>;
using M18 = Eigen::Matrix<double, 18, 18>;
using M18x12 = Eigen::Matrix<double, 18, 12>;

inline M3 Hat(const V3 &v) {
  M3 result;
  result << 0.0, -v.z(), v.y(), v.z(), 0.0, -v.x(), -v.y(), v.x(), 0.0;
  return result;
}

inline M3 Exp(const V3 &phi) {
  const double theta2 = phi.squaredNorm();
  const M3 K = Hat(phi);
  if (theta2 < 1e-12) {
    const double theta4 = theta2 * theta2;
    return M3::Identity() +
           (1.0 - theta2 / 6.0 + theta4 / 120.0) * K +
           (0.5 - theta2 / 24.0 + theta4 / 720.0) * K * K;
  }
  const double theta = std::sqrt(theta2);
  return M3::Identity() + (std::sin(theta) / theta) * K +
         ((1.0 - std::cos(theta)) / theta2) * K * K;
}

inline V3 Log(const M3 &rotation) {
  Eigen::Quaterniond q(rotation);
  q.normalize();
  if (q.w() < 0.0) {
    q.coeffs() *= -1.0;
  }
  const V3 v(q.x(), q.y(), q.z());
  const double s = v.norm();
  if (s < 1e-12) {
    return 2.0 * v;
  }
  return (2.0 * std::atan2(s, q.w()) / s) * v;
}

inline M3 RightJacobian(const V3 &angular_velocity, double dt) {
  const V3 phi = angular_velocity * dt;
  const double theta = phi.norm();
  const M3 K = Hat(phi);
  const double theta2 = theta * theta;
  const double theta4 = theta2 * theta2;
  double A;
  double B;
  if (theta < 1e-6) {
    A = 0.5 - theta2 / 24.0 + theta4 / 720.0;
    B = 1.0 / 6.0 - theta2 / 120.0 + theta4 / 5040.0;
  } else {
    A = (1.0 - std::cos(theta)) / theta2;
    B = (theta - std::sin(theta)) / (theta2 * theta);
  }
  return M3::Identity() - A * K + B * K * K;
}

struct Options {
  int num_iterations = 3;
  double quit_eps = 1e-6;
  double gyro_variance = 1e-5;
  double accelerometer_variance = 1e-2;
  double gyro_bias_variance = 1e-6;
  double accelerometer_bias_variance = 1e-4;
  double imu_scale = 1.0;
  double gravity_norm = 9.81;
};

struct ImuPair {
  V3 acceleration0 = V3::Zero();
  V3 acceleration1 = V3::Zero();
  V3 angular_velocity0 = V3::Zero();
  V3 angular_velocity1 = V3::Zero();
  double dt = 0.0;
};

struct State {
  M3 rotation = M3::Identity();
  V3 position = V3::Zero();
  V3 velocity = V3::Zero();
  V3 gyro_bias = V3::Zero();
  V3 accel_bias = V3::Zero();
  V3 gravity = V3(0.0, 0.0, -9.81);
  M18 covariance = M18::Identity();
};

inline M12 BuildNoise(const Options &options) {
  M12 noise = M12::Zero();
  noise.diagonal() << options.gyro_variance, options.gyro_variance,
      options.gyro_variance, options.accelerometer_variance,
      options.accelerometer_variance, options.accelerometer_variance,
      options.gyro_bias_variance, options.gyro_bias_variance,
      options.gyro_bias_variance, options.accelerometer_bias_variance,
      options.accelerometer_bias_variance, options.accelerometer_bias_variance;
  return noise;
}

inline void Predict(State &state, const ImuPair &imu, const Options &options) {
  const double dt = imu.dt;
  V3 acceleration =
      options.imu_scale * 0.5 * (imu.acceleration1 + imu.acceleration0);
  acceleration -= state.accel_bias;
  const V3 angular_velocity =
      0.5 * (imu.angular_velocity1 + imu.angular_velocity0) - state.gyro_bias;
  const M3 R = state.rotation;
  const M3 R_dt = R * dt;
  const M3 Jr_dt = dt * RightJacobian(angular_velocity, dt);

  M18 fx = M18::Identity();
  fx.block<3, 3>(0, 0) = Exp(-angular_velocity * dt);
  fx.block<3, 3>(0, 9) = -Jr_dt;
  fx.block<3, 3>(3, 6) = M3::Identity() * dt;
  fx.block<3, 3>(6, 0) = -R * Hat(acceleration) * dt;
  fx.block<3, 3>(6, 12) = -R_dt;
  fx.block<3, 3>(6, 15) = M3::Identity() * dt;

  M18x12 fw = M18x12::Zero();
  fw.block<3, 3>(0, 0) = -Jr_dt;
  fw.block<3, 3>(6, 3) = -R_dt;
  fw.block<3, 3>(9, 6) = M3::Identity() * dt;
  fw.block<3, 3>(12, 9) = M3::Identity() * dt;
  state.covariance = fx * state.covariance * fx.transpose() +
                     fw * BuildNoise(options) * fw.transpose();

  const V3 global_acceleration = R * acceleration + state.gravity;
  state.position += state.velocity * dt +
                    0.5 * global_acceleration * dt * dt;
  state.velocity += global_acceleration * dt;
  state.rotation = state.rotation * Exp(angular_velocity * dt);
}

inline V18 Difference(const State &predicted, const State &current) {
  V18 result = V18::Zero();
  result.segment<3>(0) = Log(predicted.rotation.transpose() * current.rotation);
  result.segment<3>(3) = current.position - predicted.position;
  result.segment<3>(6) = current.velocity - predicted.velocity;
  result.segment<3>(9) = current.gyro_bias - predicted.gyro_bias;
  result.segment<3>(12) = current.accel_bias - predicted.accel_bias;
  result.segment<3>(15) = current.gravity - predicted.gravity;
  return result;
}

inline void Apply(State &state, const V18 &increment, double gravity_norm) {
  state.rotation = state.rotation * Exp(increment.segment<3>(0));
  state.position += increment.segment<3>(3);
  state.velocity += increment.segment<3>(6);
  state.gyro_bias += increment.segment<3>(9);
  state.accel_bias += increment.segment<3>(12);
  state.gravity += increment.segment<3>(15);
  state.gravity *= gravity_norm / state.gravity.norm();
}

using ObservationCallback = std::function<void(const State &, M6 &, V6 &)>;

struct UpdateStats {
  int iterations = 0;
  bool need_converge = false;
  V18 last_increment = V18::Zero();
  M18 covariance_before_reset = M18::Zero();
};

inline UpdateStats UpdateObserve(State &state, const Options &options,
                                 const ObservationCallback &observation) {
  const State predicted = state;
  const M18 predicted_covariance = state.covariance;
  M18 posterior_covariance = M18::Zero();
  M18 gain_information = M18::Zero();
  V18 increment = V18::Zero();
  UpdateStats result;

  for (int iteration = 0; iteration < options.num_iterations; ++iteration) {
    if (iteration > 2) {
      result.need_converge = true;
    }
    M6 HtVinvH = M6::Zero();
    V6 HtVinvr = V6::Zero();
    observation(state, HtVinvH, HtVinvr);

    V18 prior_error = Difference(predicted, state);
    M18 prior_reset = M18::Identity();
    prior_reset.block<3, 3>(0, 0) =
        M3::Identity() - 0.5 * Hat(prior_error.segment<3>(0));
    const M18 prior_covariance =
        prior_reset * predicted_covariance * prior_reset.transpose();
    prior_error = prior_reset * prior_error;

    M18 HtRH = M18::Zero();
    HtRH.block<6, 6>(0, 0) = HtVinvH;
    const M18 information = prior_covariance.inverse() + HtRH;
    posterior_covariance = information.inverse();
    V18 rhs = V18::Zero();
    rhs.head<6>() = HtVinvr;
    gain_information = posterior_covariance * HtRH;
    increment = posterior_covariance * rhs +
                (gain_information - M18::Identity()) * prior_error;
    Apply(state, increment, options.gravity_norm);
    ++result.iterations;

    if (increment.lpNorm<Eigen::Infinity>() < options.quit_eps && iteration > 0) {
      break;
    }
  }

  state.covariance = posterior_covariance;
  result.covariance_before_reset = posterior_covariance;
  M18 reset = M18::Identity();
  reset.block<3, 3>(0, 0) =
      M3::Identity() - 0.5 * Hat(increment.segment<3>(0));
  state.covariance = reset * state.covariance * reset.transpose();
  state.covariance = 0.5 * (state.covariance + state.covariance.transpose());
  result.last_increment = increment;
  return result;
}

}  // namespace prob_livo_test_oracle

#endif  // PROB_LIVO_TEST_SUPER_ESKF_ORACLE_H_
