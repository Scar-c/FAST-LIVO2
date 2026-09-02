#include "test_support.h"

namespace prob_livo_test {

namespace {

Matrix19 FullDenseSPD19() {
  Matrix19 factor;
  for (int row = 0; row < 19; ++row) {
    for (int col = 0; col < 19; ++col) {
      factor(row, col) = 0.035 * (row + 1) * (col + 1) +
                         0.017 * ((row + 2 * col) % 9) +
                         (row == col ? 2.3 : 0.0);
    }
  }
  return factor * factor.transpose() + 0.2 * Matrix19::Identity();
}

Matrix19 ExpectedFullPredictCovariance(
    const StatesGroup &before, const prob_livo::ImuSample &previous,
    const prob_livo::ImuSample &current, const prob_livo::Options &options,
    bool exposure_random_walk) {
  const double dt = current.timestamp - previous.timestamp;
  const Eigen::Vector3d acceleration =
      options.imu_scale * 0.5 *
          (previous.acceleration + current.acceleration) - before.bias_a;
  const Eigen::Vector3d angular_velocity =
      0.5 * (previous.angular_velocity + current.angular_velocity) -
      before.bias_g;
  const Eigen::Matrix3d R = before.rot_end;
  const Eigen::Matrix3d R_dt = R * dt;
  const Eigen::Matrix3d Jr_dt =
      dt * prob_livo_test_oracle::RightJacobian(angular_velocity, dt);

  Matrix18 physical_fx = Matrix18::Identity();
  physical_fx.block<3, 3>(0, 0) =
      prob_livo_test_oracle::Exp(-angular_velocity * dt);
  physical_fx.block<3, 3>(0, 9) = -Jr_dt;
  physical_fx.block<3, 3>(3, 6) = Eigen::Matrix3d::Identity() * dt;
  physical_fx.block<3, 3>(6, 0) =
      -R * prob_livo_test_oracle::Hat(acceleration) * dt;
  physical_fx.block<3, 3>(6, 12) = -R_dt;
  physical_fx.block<3, 3>(6, 15) = Eigen::Matrix3d::Identity() * dt;

  Eigen::Matrix<double, 18, 12> physical_fw =
      Eigen::Matrix<double, 18, 12>::Zero();
  physical_fw.block<3, 3>(0, 0) = -Jr_dt;
  physical_fw.block<3, 3>(6, 3) = -R_dt;
  physical_fw.block<3, 3>(9, 6) = Eigen::Matrix3d::Identity() * dt;
  physical_fw.block<3, 3>(12, 9) = Eigen::Matrix3d::Identity() * dt;

  const prob_livo::HostF host_fx = prob_livo::EmbedPhysicalFx(physical_fx);
  const prob_livo::HostFw host_fw = prob_livo::EmbedPhysicalFw(physical_fw);
  const prob_livo::Matrix12 noise =
      prob_livo::ProbESKF19::NoiseCovariance(options);
  Matrix19 expected = host_fx * before.cov * host_fx.transpose() +
                      host_fw * noise * host_fw.transpose();
  if (exposure_random_walk) {
    expected(Layout::kExpo, Layout::kExpo) +=
        options.exposure_process_variance * dt * dt;
  }
  return expected;
}

}  // namespace

int RunExposureTests(TestContext &context) {
  prob_livo_test_oracle::Options oracle_options;
  oracle_options.num_iterations = 1;
  const prob_livo::Options production_options = ProductionOptions(oracle_options);

  // With P_xe=0, predict and a pose-only update leave exposure mean/variance
  // untouched.  The same invariant is exercised in the update parity suite;
  // this test also checks it with the default host covariance path.
  StatesGroup isolated_host;
  InitializeHostState(isolated_host);
  isolated_host.cov = DenseSPD19(0.37);
  prob_livo::ProbESKF19 isolated_filter(isolated_host, production_options);
  const auto isolated_previous = HostImu(
      30.0, Eigen::Vector3d(0.2, 0.1, 9.3), Eigen::Vector3d(0.2, -0.1, 0.3));
  const auto isolated_current = HostImu(
      30.04, Eigen::Vector3d(0.3, 0.3, 9.0), Eigen::Vector3d(0.3, -0.2, 0.4));
  isolated_filter.Predict(isolated_previous);
  isolated_filter.Predict(isolated_current);
  context.Check(std::abs(isolated_host.inv_expo_time - 1.7) < 1e-14,
                "isolated exposure mean changed during predict");
  context.Check(std::abs(isolated_host.cov(Layout::kExpo, Layout::kExpo) - 0.37) <
                    1e-12,
                "isolated exposure variance changed with zero cross covariance");

  // Enabling the host-compatible random walk changes only P_ee in this
  // zero-cross-covariance case.
  StatesGroup process_host;
  InitializeHostState(process_host);
  process_host.cov = DenseSPD19(0.37);
  prob_livo::Options process_options = production_options;
  process_options.exposure_random_walk_enabled = true;
  process_options.exposure_process_variance = 0.23;
  prob_livo::ProbESKF19 process_filter(process_host, process_options);
  process_filter.Predict(isolated_previous);
  process_filter.Predict(isolated_current);
  const double dt = isolated_current.timestamp - isolated_previous.timestamp;
  context.Check(std::abs(process_host.cov(Layout::kExpo, Layout::kExpo) -
                         (0.37 + 0.23 * dt * dt)) <
                    1e-12,
                "enabled exposure random walk did not use cov_inv_expo*dt^2");
  double process_cross_error = 0.0;
  for (const int index : Layout::kPhysicalIndex) {
    process_cross_error = std::max(
        process_cross_error, std::abs(process_host.cov(index, Layout::kExpo)));
  }
  context.Check(process_cross_error < 1e-12,
                "exposure random walk introduced a direct cross transition");

  // Nonzero P_xe must be propagated by F_x, not cleared.  Compare the full
  // 19D matrix against an independently assembled Super F/Q embedding.
  StatesGroup cross_host;
  InitializeHostState(cross_host);
  cross_host.cov = FullDenseSPD19();
  const Matrix19 before_covariance = cross_host.cov;
  const auto cross_previous = HostImu(
      40.0, Eigen::Vector3d(0.2, -0.3, 9.5), Eigen::Vector3d(0.7, -0.4, 0.5));
  const auto cross_current = HostImu(
      40.06, Eigen::Vector3d(0.4, -0.1, 9.0), Eigen::Vector3d(0.9, -0.2, 0.7));
  prob_livo::ProbESKF19 cross_filter(cross_host, production_options);
  cross_filter.Predict(cross_previous);
  cross_filter.Predict(cross_current);
  const Matrix19 expected_cross_covariance = ExpectedFullPredictCovariance(
      [&before_covariance]() {
        StatesGroup state;
        InitializeHostState(state);
        state.cov = before_covariance;
        return state;
      }(),
      cross_previous, cross_current, production_options, false);
  const double full_cross_error =
      (cross_host.cov - expected_cross_covariance).cwiseAbs().maxCoeff();
  context.Record("exposure.cross_full_covariance", full_cross_error);
  context.Check(full_cross_error < 3e-10,
                "nonzero exposure cross covariance was not propagated canonically");
  const double cross_norm =
      cross_host.cov.block<6, 1>(0, Layout::kExpo).norm() +
      cross_host.cov.block<6, 1>(Layout::kVel0, Layout::kExpo).norm();
  context.Check(cross_norm > 1e-5,
                "adversarial nonzero P_xe was silently removed");
  const double zero_cross_mutation =
      (cross_host.cov -
       [&before_covariance, &cross_previous, &cross_current,
        &production_options]() {
         StatesGroup state;
         InitializeHostState(state);
         state.cov = before_covariance;
         for (const int index : Layout::kPhysicalIndex) {
           state.cov(index, Layout::kExpo) = 0.0;
           state.cov(Layout::kExpo, index) = 0.0;
         }
         return ExpectedFullPredictCovariance(state, cross_previous, cross_current,
                                              production_options, false);
       }())
          .cwiseAbs()
          .maxCoeff();
  context.Record("exposure.negative_zero_cross_covariance", zero_cross_mutation);
  context.Check(zero_cross_mutation > 1e-5,
                "zeroing valid P_xe did not fail adversarial fixture");

  // LiDAR has a pose-only callback.  Cross covariance may therefore change
  // exposure indirectly, but there is no direct exposure row in H.
  StatesGroup measured_host;
  InitializeHostState(measured_host);
  measured_host.cov = FullDenseSPD19();
  const double exposure_before = measured_host.inv_expo_time;
  const double variance_before = measured_host.cov(Layout::kExpo, Layout::kExpo);
  prob_livo::ProbESKF19 measured_filter(measured_host, production_options);
  measured_filter.UpdateObserve(
      [](const StatesGroup &, Matrix6 &info, prob_livo::Vector6 &rhs) {
        info = 2.0 * Matrix6::Identity();
        rhs << 0.5, -0.2, 0.1, 0.3, -0.1, 0.2;
      });
  const double indirect_mean_change =
      std::abs(measured_host.inv_expo_time - exposure_before);
  const double indirect_variance_change =
      std::abs(measured_host.cov(Layout::kExpo, Layout::kExpo) - variance_before);
  context.Record("exposure.indirect_mean_change", indirect_mean_change);
  context.Record("exposure.indirect_variance_change", indirect_variance_change);
  context.Check(indirect_mean_change > 1e-8 || indirect_variance_change > 1e-8,
                "nonzero P_xe did not allow indirect pose-update influence");

  const double symmetry_error =
      (measured_host.cov - measured_host.cov.transpose()).cwiseAbs().maxCoeff();
  const Eigen::SelfAdjointEigenSolver<Matrix19> eigen_solver(measured_host.cov);
  const double min_eigenvalue = eigen_solver.eigenvalues().minCoeff();
  context.Record("exposure.covariance_symmetry", symmetry_error);
  context.Record("exposure.covariance_min_eigenvalue", min_eigenvalue);
  context.Check(symmetry_error < 1e-12, "posterior covariance is not symmetric");
  context.Check(std::isfinite(min_eigenvalue) && min_eigenvalue > -1e-8,
                "posterior covariance is materially non-PSD");

  context.Check(prob_livo::ClassifyCovariance(DenseSPD19()) ==
                    prob_livo::CovarianceValidity::kFiniteSymmetric,
                "finite SPD covariance was misclassified");
  Matrix19 nonfinite_covariance = DenseSPD19();
  nonfinite_covariance(0, 0) = std::numeric_limits<double>::quiet_NaN();
  context.Check(prob_livo::ClassifyCovariance(nonfinite_covariance) ==
                    prob_livo::CovarianceValidity::kNonFinite,
                "nonfinite covariance was not classified");
  Matrix19 asymmetric_covariance = DenseSPD19();
  asymmetric_covariance(0, 1) += 0.1;
  context.Check(prob_livo::ClassifyCovariance(asymmetric_covariance) ==
                    prob_livo::CovarianceValidity::kAsymmetric,
                "asymmetric covariance was not classified");
  Matrix19 negative_covariance = Matrix19::Identity();
  negative_covariance(0, 0) = -0.5;
  context.Check(prob_livo::ClassifyCovariance(negative_covariance) ==
                    prob_livo::CovarianceValidity::kNegativeEigenvalue,
                "negative-eigenvalue covariance was not classified");
  const Matrix19 singular_covariance = Matrix19::Zero();
  context.Check(prob_livo::ClassifyCovariance(singular_covariance) ==
                    prob_livo::CovarianceValidity::kSingular,
                "singular covariance was not classified");

  // Negative fixture: including exposure in a contiguous [0:7] LiDAR H block
  // directly changes P_ee even with an otherwise valid covariance.
  Matrix19 wrong_lidar_information = Matrix19::Zero();
  wrong_lidar_information.block<7, 7>(0, 0).setIdentity();
  const Matrix19 wrong_lidar_posterior =
      (measured_host.cov.inverse() + wrong_lidar_information).inverse();
  const double direct_measurement_change =
      std::abs(wrong_lidar_posterior(Layout::kExpo, Layout::kExpo) -
               measured_host.cov(Layout::kExpo, Layout::kExpo));
  context.Record("exposure.negative_lidar_exposure_row", direct_measurement_change);
  context.Check(direct_measurement_change > 1e-8,
                "direct LiDAR exposure-row mutation was not discriminated");

  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
