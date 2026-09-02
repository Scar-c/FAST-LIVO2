#include "test_support.h"

namespace prob_livo_test {

namespace {

Matrix18 MutatedNoRightJacobianCovariance(
    const OracleState &before, const prob_livo::ImuSample &previous,
    const prob_livo::ImuSample &current,
    const prob_livo_test_oracle::Options &options) {
  const double dt = current.timestamp - previous.timestamp;
  const Eigen::Vector3d acceleration =
      options.imu_scale * 0.5 *
          (previous.acceleration + current.acceleration) - before.accel_bias;
  const Eigen::Vector3d angular_velocity =
      0.5 * (previous.angular_velocity + current.angular_velocity) -
      before.gyro_bias;
  const Eigen::Matrix3d R_dt = before.rotation * dt;

  Matrix18 fx = Matrix18::Identity();
  fx.block<3, 3>(0, 0) = prob_livo_test_oracle::Exp(-angular_velocity * dt);
  fx.block<3, 3>(0, 9) = -Eigen::Matrix3d::Identity() * dt;
  fx.block<3, 3>(3, 6) = Eigen::Matrix3d::Identity() * dt;
  fx.block<3, 3>(6, 0) =
      -before.rotation * prob_livo_test_oracle::Hat(acceleration) * dt;
  fx.block<3, 3>(6, 12) = -R_dt;
  fx.block<3, 3>(6, 15) = Eigen::Matrix3d::Identity() * dt;

  Eigen::Matrix<double, 18, 12> fw = Eigen::Matrix<double, 18, 12>::Zero();
  fw.block<3, 3>(0, 0) = -Eigen::Matrix3d::Identity() * dt;
  fw.block<3, 3>(6, 3) = -R_dt;
  fw.block<3, 3>(9, 6) = Eigen::Matrix3d::Identity() * dt;
  fw.block<3, 3>(12, 9) = Eigen::Matrix3d::Identity() * dt;
  return fx * before.covariance * fx.transpose() +
         fw * prob_livo_test_oracle::BuildNoise(options) * fw.transpose();
}

Matrix18 MutatedSquaredNoiseCovariance(
    const OracleState &before, const prob_livo::ImuSample &previous,
    const prob_livo::ImuSample &current,
    const prob_livo_test_oracle::Options &options) {
  const double dt = current.timestamp - previous.timestamp;
  const Eigen::Vector3d acceleration =
      options.imu_scale * 0.5 *
          (previous.acceleration + current.acceleration) - before.accel_bias;
  const Eigen::Vector3d angular_velocity =
      0.5 * (previous.angular_velocity + current.angular_velocity) -
      before.gyro_bias;
  const Eigen::Matrix3d R = before.rotation;
  const Eigen::Matrix3d R_dt = R * dt;
  const Eigen::Matrix3d Jr_dt =
      dt * prob_livo_test_oracle::RightJacobian(angular_velocity, dt);
  Matrix18 fx = Matrix18::Identity();
  fx.block<3, 3>(0, 0) = prob_livo_test_oracle::Exp(-angular_velocity * dt);
  fx.block<3, 3>(0, 9) = -Jr_dt;
  fx.block<3, 3>(3, 6) = Eigen::Matrix3d::Identity() * dt;
  fx.block<3, 3>(6, 0) = -R * prob_livo_test_oracle::Hat(acceleration) * dt;
  fx.block<3, 3>(6, 12) = -R_dt;
  fx.block<3, 3>(6, 15) = Eigen::Matrix3d::Identity() * dt;
  Eigen::Matrix<double, 18, 12> fw = Eigen::Matrix<double, 18, 12>::Zero();
  fw.block<3, 3>(0, 0) = -Jr_dt;
  fw.block<3, 3>(6, 3) = -R_dt;
  fw.block<3, 3>(9, 6) = Eigen::Matrix3d::Identity() * dt;
  fw.block<3, 3>(12, 9) = Eigen::Matrix3d::Identity() * dt;
  const Eigen::Matrix<double, 12, 12> noise =
      prob_livo_test_oracle::BuildNoise(options);
  return fx * before.covariance * fx.transpose() +
         fw * noise.cwiseProduct(noise) * fw.transpose();
}

Matrix18 MutatedNoisePlacementCovariance(
    const OracleState &before, const prob_livo::ImuSample &previous,
    const prob_livo::ImuSample &current,
    const prob_livo_test_oracle::Options &options) {
  const double dt = current.timestamp - previous.timestamp;
  const Eigen::Vector3d acceleration =
      options.imu_scale * 0.5 *
          (previous.acceleration + current.acceleration) - before.accel_bias;
  const Eigen::Vector3d angular_velocity =
      0.5 * (previous.angular_velocity + current.angular_velocity) -
      before.gyro_bias;
  const Eigen::Matrix3d R = before.rotation;
  const Eigen::Matrix3d R_dt = R * dt;
  const Eigen::Matrix3d Jr_dt =
      dt * prob_livo_test_oracle::RightJacobian(angular_velocity, dt);
  Matrix18 fx = Matrix18::Identity();
  fx.block<3, 3>(0, 0) = prob_livo_test_oracle::Exp(-angular_velocity * dt);
  fx.block<3, 3>(0, 9) = -Jr_dt;
  fx.block<3, 3>(3, 6) = Eigen::Matrix3d::Identity() * dt;
  fx.block<3, 3>(6, 0) = -R * prob_livo_test_oracle::Hat(acceleration) * dt;
  fx.block<3, 3>(6, 12) = -R_dt;
  fx.block<3, 3>(6, 15) = Eigen::Matrix3d::Identity() * dt;
  Eigen::Matrix<double, 18, 12> fw = Eigen::Matrix<double, 18, 12>::Zero();
  fw.block<3, 3>(0, 0) = -Jr_dt;
  fw.block<3, 3>(3, 3) = -R_dt;  // mutation: accelerometer noise drives p
  fw.block<3, 3>(9, 6) = Eigen::Matrix3d::Identity() * dt;
  fw.block<3, 3>(12, 9) = Eigen::Matrix3d::Identity() * dt;
  return fx * before.covariance * fx.transpose() +
         fw * prob_livo_test_oracle::BuildNoise(options) * fw.transpose();
}

}  // namespace

int RunPredictTests(TestContext &context) {
  prob_livo_test_oracle::Options oracle_options;
  oracle_options.gyro_variance = 0.0007;
  oracle_options.accelerometer_variance = 0.004;
  oracle_options.gyro_bias_variance = 0.00003;
  oracle_options.accelerometer_bias_variance = 0.0002;
  oracle_options.imu_scale = 1.13;
  oracle_options.gravity_norm = 9.81;
  const prob_livo::Options production_options = ProductionOptions(oracle_options);

  StatesGroup host;
  InitializeHostState(host);
  OracleState oracle = ToOracle(host);
  host.cov = prob_livo::EmbedPhysicalCovariance(oracle.covariance);
  host.cov(Layout::kExpo, Layout::kExpo) = 0.37;
  prob_livo::ProbESKF19 filter(host, production_options);

  // The first pair uses zero gyro; later pairs use finite angular motion,
  // nonidentity acceleration orientation, small dt, and a realistic dt.
  const std::vector<prob_livo::ImuSample> samples = {
      HostImu(10.000, Eigen::Vector3d(0.2, -0.4, 9.6),
              Eigen::Vector3d::Zero()),
      HostImu(10.010, Eigen::Vector3d(0.3, -0.2, 9.4),
              Eigen::Vector3d::Zero()),
      HostImu(10.080, Eigen::Vector3d(0.5, 0.1, 9.1),
              Eigen::Vector3d(0.8, -0.3, 0.4)),
      HostImu(10.093, Eigen::Vector3d(0.7, 0.2, 8.9),
              Eigen::Vector3d(0.9, -0.2, 0.45)),
      HostImu(10.143, Eigen::Vector3d(0.1, 0.5, 9.3),
              Eigen::Vector3d(0.4, 0.2, -0.15))};

  context.Check(!filter.Predict(samples.front()),
                "first IMU sample should establish history only");
  for (std::size_t index = 1; index < samples.size(); ++index) {
    const bool predicted = filter.Predict(samples[index]);
    context.Check(predicted, "valid IMU pair was not propagated");
    prob_livo_test_oracle::Predict(
        oracle, OracleImu(samples[index - 1], samples[index]), oracle_options);
    CheckState(context, oracle, host, "predict.step" + std::to_string(index),
               2e-11);
    CheckPhysicalCovariance(context, oracle.covariance, host.cov,
                            "predict.step" + std::to_string(index), 2e-10);
    context.Record("predict.exposure_mean",
                   host.inv_expo_time - 1.7);
    context.Record("predict.exposure_variance",
                   host.cov(Layout::kExpo, Layout::kExpo) - 0.37);
  }
  context.Check(std::abs(host.inv_expo_time - 1.7) < 1e-14,
                "frozen exposure mean changed during predict");
  context.Check(std::abs(host.cov(Layout::kExpo, Layout::kExpo) - 0.37) < 1e-12,
                "frozen exposure covariance changed during predict");
  context.Check(std::abs(filter.current_time() - samples.back().timestamp) < 1e-14,
                "filter timestamp did not follow propagated IMU");

  // Negative covariance fixtures discriminate the right-Jacobian and noise
  // placement/squaring requirements from a superficially similar FAST model.
  StatesGroup negative_host;
  InitializeHostState(negative_host);
  OracleState negative_oracle = ToOracle(negative_host);
  const auto previous = HostImu(20.0, Eigen::Vector3d(0.1, 0.4, 9.2),
                                Eigen::Vector3d(1.3, -0.8, 0.6));
  const auto current = HostImu(20.07, Eigen::Vector3d(0.3, 0.6, 9.0),
                                Eigen::Vector3d(1.5, -0.7, 0.7));
  prob_livo_test_oracle::Predict(
      negative_oracle, OracleImu(previous, current), oracle_options);
  const Matrix18 expected_covariance = negative_oracle.covariance;
  const double right_jacobian_mutation =
      (expected_covariance -
       MutatedNoRightJacobianCovariance(ToOracle(negative_host), previous,
                                        current, oracle_options))
          .cwiseAbs()
          .maxCoeff();
  const double noise_square_mutation =
      (expected_covariance -
       MutatedSquaredNoiseCovariance(ToOracle(negative_host), previous, current,
                                     oracle_options))
          .cwiseAbs()
          .maxCoeff();
  const double noise_placement_mutation =
      (expected_covariance -
       MutatedNoisePlacementCovariance(ToOracle(negative_host), previous, current,
                                       oracle_options))
          .cwiseAbs()
          .maxCoeff();
  context.Record("predict.negative_no_right_jacobian", right_jacobian_mutation);
  context.Record("predict.negative_squared_noise", noise_square_mutation);
  context.Record("predict.negative_noise_placement", noise_placement_mutation);
  context.Check(right_jacobian_mutation > 1e-8,
                "right-Jacobian omission was not discriminated");
  context.Check(noise_square_mutation > 1e-8,
                "noise squaring mutation was not discriminated");
  context.Check(noise_placement_mutation > 1e-8,
                "accelerometer noise placement mutation was not discriminated");

  // Invalid timestamps are rejected without touching the nominal state.
  const StatesGroup before_invalid = host;
  context.Check(!filter.Predict(HostImu(10.1, Eigen::Vector3d::Constant(
                                               std::numeric_limits<double>::quiet_NaN()),
                                           Eigen::Vector3d::Zero())),
                "non-finite IMU sample was accepted");
  context.Check(prob_livo::LogSO3(before_invalid.rot_end.transpose() * host.rot_end)
                        .norm() < 1e-14 &&
                    (before_invalid.cov - host.cov).cwiseAbs().maxCoeff() < 1e-14,
                "invalid IMU sample changed state");

  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
