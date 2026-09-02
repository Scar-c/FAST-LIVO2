#ifndef PROB_LIVO_TEST_SUPPORT_H_
#define PROB_LIVO_TEST_SUPPORT_H_

#include "prob_livo/prob_eskf19.h"
#include "oracle/super_eskf_oracle.h"

#include <Eigen/Geometry>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace prob_livo_test {

using prob_livo::Layout;
using prob_livo::Matrix6;
using prob_livo::Matrix18;
using prob_livo::Matrix19;
using prob_livo::Vector18;
using prob_livo::Vector19;
using OracleState = prob_livo_test_oracle::State;

struct TestContext {
  int checks = 0;
  std::vector<std::string> failures;
  std::map<std::string, double> metrics;

  void Check(bool condition, const std::string &message) {
    ++checks;
    if (!condition) {
      failures.push_back(message);
    }
  }

  void Record(const std::string &name, double value) {
    auto it = metrics.find(name);
    if (it == metrics.end()) {
      metrics.emplace(name, std::abs(value));
    } else {
      it->second = std::max(it->second, std::abs(value));
    }
  }

  bool Passed() const { return failures.empty(); }

  void Print(const std::string &suite) const {
    std::cout << "[" << (Passed() ? "PASS" : "FAIL") << "] " << suite
              << " checks=" << checks << "\n";
    for (const auto &metric : metrics) {
      std::cout << "  " << metric.first << "=" << std::setprecision(16)
                << metric.second << "\n";
    }
    for (const auto &failure : failures) {
      std::cout << "  FAILURE: " << failure << "\n";
    }
  }
};

inline Matrix18 DenseSPD18() {
  Eigen::Matrix<double, 18, 18> factor;
  for (int row = 0; row < 18; ++row) {
    for (int col = 0; col < 18; ++col) {
      factor(row, col) = 0.07 * (row + 1) * (col + 2) +
                         0.013 * ((row + 3 * col) % 7) +
                         (row == col ? 2.0 : 0.0);
    }
  }
  return factor * factor.transpose() + 0.25 * Matrix18::Identity();
}

inline Matrix19 DenseSPD19(double exposure_variance = 0.37) {
  Matrix19 result = Matrix19::Zero();
  const Matrix18 physical = DenseSPD18();
  for (int row = 0; row < 18; ++row) {
    for (int col = 0; col < 18; ++col) {
      result(Layout::kPhysicalIndex[static_cast<std::size_t>(row)],
             Layout::kPhysicalIndex[static_cast<std::size_t>(col)]) =
          physical(row, col);
    }
  }
  result(Layout::kExpo, Layout::kExpo) = exposure_variance;
  return result;
}

inline Eigen::Matrix3d InitialRotation() {
  const Eigen::Vector3d axis(0.2, -0.1, 0.3);
  const double angle = 0.45;
  return Eigen::AngleAxisd(angle, axis.normalized()).toRotationMatrix();
}

inline void InitializeHostState(StatesGroup &state) {
  state.rot_end = InitialRotation();
  state.pos_end = Eigen::Vector3d(1.2, -0.7, 0.4);
  state.vel_end = Eigen::Vector3d(-0.3, 0.8, 0.5);
  state.inv_expo_time = 1.7;
  state.bias_g = Eigen::Vector3d(0.012, -0.021, 0.009);
  state.bias_a = Eigen::Vector3d(-0.14, 0.07, 0.05);
  state.gravity = Eigen::Vector3d(0.4, -0.2, -9.78);
  state.gravity *= 9.81 / state.gravity.norm();
  state.cov = DenseSPD19();
}

inline OracleState ToOracle(const StatesGroup &host) {
  OracleState oracle;
  oracle.rotation = host.rot_end;
  oracle.position = host.pos_end;
  oracle.velocity = host.vel_end;
  oracle.gyro_bias = host.bias_g;
  oracle.accel_bias = host.bias_a;
  oracle.gravity = host.gravity;
  oracle.covariance = prob_livo::ExtractPhysicalCovariance(host.cov);
  return oracle;
}

inline void CheckState(TestContext &context, const OracleState &oracle,
                       const StatesGroup &host, const std::string &prefix,
                       double tolerance) {
  const double rotation_error = prob_livo::LogSO3(oracle.rotation.transpose() *
                                                   host.rot_end)
                                    .norm();
  const double position_error = (oracle.position - host.pos_end).norm();
  const double velocity_error = (oracle.velocity - host.vel_end).norm();
  const double gyro_bias_error = (oracle.gyro_bias - host.bias_g).norm();
  const double accel_bias_error = (oracle.accel_bias - host.bias_a).norm();
  const double gravity_error = (oracle.gravity - host.gravity).norm();
  context.Record(prefix + ".rotation", rotation_error);
  context.Record(prefix + ".position", position_error);
  context.Record(prefix + ".velocity", velocity_error);
  context.Record(prefix + ".gyro_bias", gyro_bias_error);
  context.Record(prefix + ".accel_bias", accel_bias_error);
  context.Record(prefix + ".gravity", gravity_error);
  context.Check(rotation_error <= tolerance, prefix + " rotation mismatch");
  context.Check(position_error <= tolerance, prefix + " position mismatch");
  context.Check(velocity_error <= tolerance, prefix + " velocity mismatch");
  context.Check(gyro_bias_error <= tolerance, prefix + " gyro bias mismatch");
  context.Check(accel_bias_error <= tolerance, prefix + " accel bias mismatch");
  context.Check(gravity_error <= tolerance, prefix + " gravity mismatch");
}

inline void CheckPhysicalCovariance(TestContext &context,
                                    const Matrix18 &oracle_covariance,
                                    const Matrix19 &host_covariance,
                                    const std::string &prefix,
                                    double tolerance) {
  const Matrix18 host_physical =
      prob_livo::ExtractPhysicalCovariance(host_covariance);
  const double error = (oracle_covariance - host_physical).cwiseAbs().maxCoeff();
  context.Record(prefix + ".physical_covariance", error);
  context.Check(error <= tolerance, prefix + " physical covariance mismatch");
}

inline prob_livo::Options ProductionOptions(
    const prob_livo_test_oracle::Options &oracle_options) {
  prob_livo::Options options;
  options.num_iterations = oracle_options.num_iterations;
  options.quit_eps = oracle_options.quit_eps;
  options.gyro_variance = oracle_options.gyro_variance;
  options.accelerometer_variance = oracle_options.accelerometer_variance;
  options.gyro_bias_variance = oracle_options.gyro_bias_variance;
  options.accelerometer_bias_variance = oracle_options.accelerometer_bias_variance;
  options.imu_scale = oracle_options.imu_scale;
  options.gravity_norm = oracle_options.gravity_norm;
  return options;
}

inline prob_livo::ImuSample HostImu(double timestamp, const Eigen::Vector3d &acc,
                                    const Eigen::Vector3d &gyr) {
  prob_livo::ImuSample sample;
  sample.timestamp = timestamp;
  sample.acceleration = acc;
  sample.angular_velocity = gyr;
  return sample;
}

inline prob_livo_test_oracle::ImuPair OracleImu(
    const prob_livo::ImuSample &previous, const prob_livo::ImuSample &current) {
  prob_livo_test_oracle::ImuPair pair;
  pair.acceleration0 = previous.acceleration;
  pair.acceleration1 = current.acceleration;
  pair.angular_velocity0 = previous.angular_velocity;
  pair.angular_velocity1 = current.angular_velocity;
  pair.dt = current.timestamp - previous.timestamp;
  return pair;
}

inline double MaxAbs(const Matrix19 &matrix) {
  return matrix.cwiseAbs().maxCoeff();
}

}  // namespace prob_livo_test

#endif  // PROB_LIVO_TEST_SUPPORT_H_
