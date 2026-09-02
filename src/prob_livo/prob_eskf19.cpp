#include "prob_livo/prob_eskf19.h"

#include <algorithm>

namespace prob_livo {

void ApplySuperLioIncrement19(StatesGroup &state, const Vector19 &increment,
                              double gravity_norm) {
  state.rot_end = state.rot_end * ExpSO3(increment.segment<3>(Layout::kRot0));
  state.pos_end += increment.segment<3>(Layout::kPos0);
  state.inv_expo_time += increment(Layout::kExpo);
  state.vel_end += increment.segment<3>(Layout::kVel0);
  state.bias_g += increment.segment<3>(Layout::kBg0);
  state.bias_a += increment.segment<3>(Layout::kBa0);
  state.gravity += increment.segment<3>(Layout::kGrav0);

  const double gravity_length = state.gravity.norm();
  if (gravity_length > std::numeric_limits<double>::epsilon() &&
      std::isfinite(gravity_length)) {
    state.gravity *= gravity_norm / gravity_length;
  }
}

Vector19 StateDifference19(const StatesGroup &predicted,
                           const StatesGroup &current) {
  Vector19 difference = Vector19::Zero();
  difference.segment<3>(Layout::kRot0) =
      LogSO3(predicted.rot_end.transpose() * current.rot_end);
  difference.segment<3>(Layout::kPos0) = current.pos_end - predicted.pos_end;
  difference(Layout::kExpo) = current.inv_expo_time - predicted.inv_expo_time;
  difference.segment<3>(Layout::kVel0) = current.vel_end - predicted.vel_end;
  difference.segment<3>(Layout::kBg0) = current.bias_g - predicted.bias_g;
  difference.segment<3>(Layout::kBa0) = current.bias_a - predicted.bias_a;
  difference.segment<3>(Layout::kGrav0) = current.gravity - predicted.gravity;
  return difference;
}

CovarianceValidity ClassifyCovariance(const Matrix19 &covariance,
                                      double symmetry_tolerance,
                                      double singular_tolerance,
                                      double negative_tolerance) {
  if (!covariance.allFinite()) {
    return CovarianceValidity::kNonFinite;
  }
  if ((covariance - covariance.transpose()).cwiseAbs().maxCoeff() >
      symmetry_tolerance) {
    return CovarianceValidity::kAsymmetric;
  }
  const Eigen::SelfAdjointEigenSolver<Matrix19> eigen_solver(covariance);
  if (eigen_solver.info() != Eigen::Success) {
    return CovarianceValidity::kSingular;
  }
  const double minimum_eigenvalue = eigen_solver.eigenvalues().minCoeff();
  if (minimum_eigenvalue < -negative_tolerance) {
    return CovarianceValidity::kNegativeEigenvalue;
  }
  if (minimum_eigenvalue <= singular_tolerance) {
    return CovarianceValidity::kSingular;
  }
  return CovarianceValidity::kFiniteSymmetric;
}

ProbESKF19::ProbESKF19(StatesGroup &state, Options options)
    : state_(state), options_(options) {}

void ProbESKF19::ResetImuHistory() {
  has_last_imu_ = false;
  last_imu_time_ = -1.0;
  current_time_ = 0.0;
  last_imu_ = ImuSample();
}

void ProbESKF19::SetObservationWindow(double last_observation_time,
                                      double current_observation_time) {
  last_observation_time_ = last_observation_time;
  current_observation_time_ = current_observation_time;
}

Matrix12 ProbESKF19::NoiseCovariance(const Options &options) {
  Matrix12 noise = Matrix12::Zero();
  noise.diagonal().segment<3>(0).setConstant(options.gyro_variance);
  noise.diagonal().segment<3>(3).setConstant(options.accelerometer_variance);
  noise.diagonal().segment<3>(6).setConstant(options.gyro_bias_variance);
  noise.diagonal().segment<3>(9).setConstant(options.accelerometer_bias_variance);
  return noise;
}

bool ProbESKF19::Predict(const ImuSample &imu) {
  if (!imu.acceleration.allFinite() || !imu.angular_velocity.allFinite() ||
      !std::isfinite(imu.timestamp)) {
    return false;
  }

  if (!has_last_imu_) {
    last_imu_ = imu;
    last_imu_time_ = imu.timestamp;
    current_time_ = imu.timestamp;
    has_last_imu_ = true;
    return false;
  }

  if (imu.timestamp <= last_observation_time_) {
    last_imu_ = imu;
    last_imu_time_ = imu.timestamp;
    current_time_ = imu.timestamp;
    return false;
  }

  current_time_ = imu.timestamp;
  double dt = imu.timestamp - last_imu_time_;
  if (last_imu_time_ < last_observation_time_) {
    dt = imu.timestamp - last_observation_time_;
  } else if (imu.timestamp > current_observation_time_) {
    dt = current_observation_time_ - last_imu_time_;
    current_time_ = current_observation_time_;
  }

  if (!(dt >= 0.0) || !std::isfinite(dt)) {
    return false;
  }

  Eigen::Vector3d acceleration =
      options_.imu_scale * 0.5 * (imu.acceleration + last_imu_.acceleration);
  acceleration -= state_.bias_a;
  const Eigen::Vector3d angular_velocity =
      0.5 * (imu.angular_velocity + last_imu_.angular_velocity) - state_.bias_g;

  const Eigen::Matrix3d R = state_.rot_end;
  const Eigen::Matrix3d R_dt = R * dt;
  const Eigen::Matrix3d Jr_dt =
      dt * RightJacobianSO3(angular_velocity, dt);

  Matrix18 physical_fx = Matrix18::Identity();
  physical_fx.block<3, 3>(0, 0) = ExpSO3(-angular_velocity * dt);
  physical_fx.block<3, 3>(0, 9) = -Jr_dt;
  physical_fx.block<3, 3>(3, 6) = Eigen::Matrix3d::Identity() * dt;
  physical_fx.block<3, 3>(6, 0) = -R * HatSO3(acceleration) * dt;
  physical_fx.block<3, 3>(6, 12) = -R_dt;
  physical_fx.block<3, 3>(6, 15) = Eigen::Matrix3d::Identity() * dt;

  Matrix18x12 physical_fw = Matrix18x12::Zero();
  physical_fw.block<3, 3>(0, 0) = -Jr_dt;
  physical_fw.block<3, 3>(6, 3) = -R_dt;
  physical_fw.block<3, 3>(9, 6) = Eigen::Matrix3d::Identity() * dt;
  physical_fw.block<3, 3>(12, 9) = Eigen::Matrix3d::Identity() * dt;

  const HostF host_fx = EmbedPhysicalFx(physical_fx);
  const HostFw host_fw = EmbedPhysicalFw(physical_fw);
  const Matrix12 noise12 = NoiseCovariance(options_);

  state_.cov = host_fx * state_.cov * host_fx.transpose() +
               host_fw * noise12 * host_fw.transpose();
  if (options_.exposure_random_walk_enabled) {
    state_.cov(Layout::kExpo, Layout::kExpo) +=
        options_.exposure_process_variance * dt * dt;
  }

  // The nominal update intentionally uses the pre-rotation R, as in Super's
  // active ESKF.cpp implementation.
  const Eigen::Vector3d global_acceleration = R * acceleration + state_.gravity;
  state_.pos_end += state_.vel_end * dt + 0.5 * global_acceleration * dt * dt;
  state_.vel_end += global_acceleration * dt;
  state_.rot_end = state_.rot_end * ExpSO3(angular_velocity * dt);

  last_imu_ = imu;
  last_imu_time_ = imu.timestamp;
  return true;
}

bool ProbESKF19::UpdateObserve(const ObservationCallback &observation) {
  if (!observation || options_.num_iterations <= 0) {
    return false;
  }

  const StatesGroup predicted = state_;
  const Matrix19 predicted_covariance = state_.cov;

  Matrix19 posterior_covariance = Matrix19::Zero();
  Matrix19 reset_covariance = Matrix19::Zero();
  Matrix19 gain_information = Matrix19::Zero();
  Vector19 increment = Vector19::Zero();
  need_converge_ = false;
  last_update_iterations_ = 0;

  for (int iteration = 0; iteration < options_.num_iterations; ++iteration) {
    if (iteration > 2) {
      need_converge_ = true;
    }

    Matrix6 HtVinvH = Matrix6::Zero();
    Vector6 HtVinvr = Vector6::Zero();
    observation(state_, HtVinvH, HtVinvr);

    Vector19 prior_error = StateDifference19(predicted, state_);
    Matrix19 prior_reset = Matrix19::Identity();
    prior_reset.block<3, 3>(Layout::kRot0, Layout::kRot0) =
        Eigen::Matrix3d::Identity() -
        0.5 * HatSO3(prior_error.segment<3>(Layout::kRot0));

    const Matrix19 prior_covariance =
        prior_reset * predicted_covariance * prior_reset.transpose();
    prior_error = prior_reset * prior_error;

    Matrix19 HtRH = Matrix19::Zero();
    HtRH.block<6, 6>(0, 0) = HtVinvH;

    const Matrix19 information = prior_covariance.inverse() + HtRH;
    posterior_covariance = information.inverse();
    Vector19 rhs = Vector19::Zero();
    rhs.head<6>() = HtVinvr;
    gain_information = posterior_covariance * HtRH;

    increment = posterior_covariance * rhs +
                (gain_information - Matrix19::Identity()) * prior_error;
    ApplySuperLioIncrement19(state_, increment, options_.gravity_norm);
    ++last_update_iterations_;

    if (increment.lpNorm<Eigen::Infinity>() < options_.quit_eps && iteration > 0) {
      break;
    }
  }

  state_.cov = posterior_covariance;
  reset_covariance = Matrix19::Identity();
  reset_covariance.block<3, 3>(Layout::kRot0, Layout::kRot0) =
      Eigen::Matrix3d::Identity() -
      0.5 * HatSO3(increment.segment<3>(Layout::kRot0));
  state_.cov = reset_covariance * state_.cov * reset_covariance.transpose();
  state_.cov = 0.5 * (state_.cov + state_.cov.transpose());

  last_increment_ = increment;
  if (std::isfinite(current_observation_time_)) {
    last_observation_time_ = current_observation_time_;
  }
  return true;
}

}  // namespace prob_livo
