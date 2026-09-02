#ifndef PROB_LIVO_TEST_I2_SUPPORT_H_
#define PROB_LIVO_TEST_I2_SUPPORT_H_

#include "prob_livo/prob_imu_adapter.h"
#include "oracle/super_imu_oracle.h"
#include "test_support.h"

#include <cmath>
#include <vector>

namespace prob_livo_test {

inline sensor_msgs::Imu::ConstPtr ToRosImu(const prob_livo::ImuSample &sample) {
  sensor_msgs::Imu::Ptr message(new sensor_msgs::Imu());
  message->header.stamp = ros::Time(sample.timestamp);
  message->linear_acceleration.x = sample.acceleration.x();
  message->linear_acceleration.y = sample.acceleration.y();
  message->linear_acceleration.z = sample.acceleration.z();
  message->angular_velocity.x = sample.angular_velocity.x();
  message->angular_velocity.y = sample.angular_velocity.y();
  message->angular_velocity.z = sample.angular_velocity.z();
  return message;
}

inline LidarMeasureGroup MakeEpoch(
    double epoch_start, double epoch_end, double point_time_origin,
    const std::vector<prob_livo::ImuSample> &imu_samples,
    const std::vector<std::pair<double, int>> &points) {
  LidarMeasureGroup measures;
  measures.last_lio_update_time = epoch_start;
  measures.lidar_frame_beg_time = point_time_origin;
  measures.lidar_frame_end_time = epoch_end;
  measures.measures.emplace_back();
  measures.measures.back().lio_time = epoch_end;
  for (const auto &sample : imu_samples) {
    measures.measures.back().imu.push_back(ToRosImu(sample));
  }
  for (const auto &point : points) {
    PointType value;
    value.x = static_cast<float>(point.second);
    value.y = static_cast<float>(-point.second);
    value.z = static_cast<float>(0.5 * point.second);
    value.intensity = static_cast<float>(1000 + point.second);
    value.curvature = static_cast<float>(
        (point.first - point_time_origin) * 1000.0);
    value.normal_x = static_cast<float>(point.second);
    value.normal_y = static_cast<float>(-point.second);
    value.normal_z = static_cast<float>(2 * point.second);
    measures.pcl_proc_cur->points.push_back(value);
  }
  measures.lidar = measures.pcl_proc_cur;
  return measures;
}

inline std::vector<prob_livo::ImuSample> MakeImuSequence(
    double start_time, int count, const Eigen::Vector3d &acceleration,
    const Eigen::Vector3d &angular_velocity, double dt = 0.01) {
  std::vector<prob_livo::ImuSample> samples;
  samples.reserve(static_cast<std::size_t>(count));
  for (int index = 0; index < count; ++index) {
    samples.push_back(prob_livo::ImuSample{
        start_time + index * dt, acceleration,
        angular_velocity});
  }
  return samples;
}

inline void CheckMatrix3(TestContext &context, const Eigen::Matrix3d &expected,
                         const Eigen::Matrix3d &actual,
                         const std::string &name, double tolerance) {
  const double error = (expected - actual).cwiseAbs().maxCoeff();
  context.Record(name, error);
  context.Check(error <= tolerance, name + " mismatch");
}

inline void CheckVector3(TestContext &context, const Eigen::Vector3d &expected,
                         const Eigen::Vector3d &actual,
                         const std::string &name, double tolerance) {
  const double error = (expected - actual).norm();
  context.Record(name, error);
  context.Check(error <= tolerance, name + " mismatch");
}

inline void SetOracleFromInit(prob_livo_test_oracle::State &oracle,
                              const prob_livo_test_oracle::InitResult &init,
                              double gravity_norm = 9.7946) {
  oracle.rotation = init.rotation;
  oracle.position = init.position;
  oracle.velocity.setZero();
  oracle.gyro_bias = init.mean_gyro;
  oracle.accel_bias.setZero();
  oracle.gravity = Eigen::Vector3d(0.0, 0.0, -gravity_norm);
  oracle.covariance = init.covariance;
}

inline prob_livo::ProbImuAdapter::Options AdapterOptions(
    double gravity_norm = 9.7946) {
  prob_livo::ProbImuAdapter::Options options;
  options.gravity_norm = gravity_norm;
  options.time_tolerance = 2e-8;
  return options;
}

inline prob_livo::Options FilterOptions(double gravity_norm = 9.7946) {
  prob_livo::Options options;
  options.gravity_norm = gravity_norm;
  options.num_iterations = 3;
  options.quit_eps = 1e-6;
  return options;
}

}  // namespace prob_livo_test

#endif  // PROB_LIVO_TEST_I2_SUPPORT_H_
