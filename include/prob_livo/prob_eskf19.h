#ifndef PROB_LIVO_PROB_ESKF19_H_
#define PROB_LIVO_PROB_ESKF19_H_

#include "common_lib.h"

#include <array>
#include <cmath>
#include <Eigen/Eigenvalues>
#include <functional>
#include <limits>

namespace prob_livo {

/**
 * Host-layout constants for the single FAST-LIVO2 state.
 *
 * The six physical Super blocks are deliberately non-contiguous in the host
 * vector because inverse exposure occupies index 6.  All translations between
 * those representations belong here; callers must not hand-code an offset.
 */
struct Layout {
  static constexpr int kRot0 = 0;
  static constexpr int kPos0 = 3;
  static constexpr int kExpo = 6;
  static constexpr int kVel0 = 7;
  static constexpr int kBg0 = 10;
  static constexpr int kBa0 = 13;
  static constexpr int kGrav0 = 16;
  static constexpr int kDim19 = 19;
  static constexpr int kDim18 = 18;
  static constexpr int kNoise12 = 12;

  inline static constexpr std::array<int, kDim18> kPhysicalIndex = {
      0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
};

using Vector19 = Eigen::Matrix<double, Layout::kDim19, 1>;
using Matrix19 = Eigen::Matrix<double, Layout::kDim19, Layout::kDim19>;
using Vector18 = Eigen::Matrix<double, Layout::kDim18, 1>;
using Matrix18 = Eigen::Matrix<double, Layout::kDim18, Layout::kDim18>;
using Matrix18x12 = Eigen::Matrix<double, Layout::kDim18, Layout::kNoise12>;
using Matrix12 = Eigen::Matrix<double, Layout::kNoise12, Layout::kNoise12>;
using Matrix6 = Eigen::Matrix<double, 6, 6>;
using Vector6 = Eigen::Matrix<double, 6, 1>;

using HostF = Matrix19;
using HostFw = Eigen::Matrix<double, Layout::kDim19, Layout::kNoise12>;

/** Embed/extract a physical Super vector in the host x19 layout. */
inline Vector19 EmbedPhysicalVector(const Vector18 &physical) {
  Vector19 host = Vector19::Zero();
  for (int i = 0; i < Layout::kDim18; ++i) {
    host(Layout::kPhysicalIndex[static_cast<std::size_t>(i)]) = physical(i);
  }
  return host;
}

inline Vector18 ExtractPhysicalVector(const Vector19 &host) {
  Vector18 physical = Vector18::Zero();
  for (int i = 0; i < Layout::kDim18; ++i) {
    physical(i) = host(Layout::kPhysicalIndex[static_cast<std::size_t>(i)]);
  }
  return physical;
}

/** Embed/extract physical covariance without dropping off-diagonal terms. */
inline Matrix19 EmbedPhysicalCovariance(const Matrix18 &physical) {
  Matrix19 host = Matrix19::Zero();
  host(Layout::kExpo, Layout::kExpo) = 0.0;
  for (int i = 0; i < Layout::kDim18; ++i) {
    for (int j = 0; j < Layout::kDim18; ++j) {
      host(Layout::kPhysicalIndex[static_cast<std::size_t>(i)],
           Layout::kPhysicalIndex[static_cast<std::size_t>(j)]) = physical(i, j);
    }
  }
  return host;
}

inline Matrix18 ExtractPhysicalCovariance(const Matrix19 &host) {
  Matrix18 physical = Matrix18::Zero();
  for (int i = 0; i < Layout::kDim18; ++i) {
    for (int j = 0; j < Layout::kDim18; ++j) {
      physical(i, j) = host(Layout::kPhysicalIndex[static_cast<std::size_t>(i)],
                            Layout::kPhysicalIndex[static_cast<std::size_t>(j)]);
    }
  }
  return physical;
}

/** Embed the Super covariance transition and process-noise transition. */
inline HostF EmbedPhysicalFx(const Matrix18 &physical_fx) {
  HostF host = HostF::Identity();
  for (int i = 0; i < Layout::kDim18; ++i) {
    for (int j = 0; j < Layout::kDim18; ++j) {
      host(Layout::kPhysicalIndex[static_cast<std::size_t>(i)],
           Layout::kPhysicalIndex[static_cast<std::size_t>(j)]) = physical_fx(i, j);
    }
  }
  return host;
}

inline HostFw EmbedPhysicalFw(const Matrix18x12 &physical_fw) {
  HostFw host = HostFw::Zero();
  for (int i = 0; i < Layout::kDim18; ++i) {
    for (int j = 0; j < Layout::kNoise12; ++j) {
      host(Layout::kPhysicalIndex[static_cast<std::size_t>(i)], j) = physical_fw(i, j);
    }
  }
  return host;
}

inline Eigen::Matrix3d HatSO3(const Eigen::Vector3d &v) {
  Eigen::Matrix3d result;
  result << 0.0, -v.z(), v.y(), v.z(), 0.0, -v.x(), -v.y(), v.x(), 0.0;
  return result;
}

/** Canonical right retraction exponential, with small-angle stable coefficients. */
inline Eigen::Matrix3d ExpSO3(const Eigen::Vector3d &phi) {
  const double theta2 = phi.squaredNorm();
  const Eigen::Matrix3d K = HatSO3(phi);
  double A;
  double B;
  if (theta2 < 1e-12) {
    A = 1.0 - theta2 / 6.0 + theta2 * theta2 / 120.0;
    B = 0.5 - theta2 / 24.0 + theta2 * theta2 / 720.0;
  } else {
    const double theta = std::sqrt(theta2);
    A = std::sin(theta) / theta;
    B = (1.0 - std::cos(theta)) / theta2;
  }
  return Eigen::Matrix3d::Identity() + A * K + B * K * K;
}

/** Canonical logarithm used for the Super right-error prior. */
inline Eigen::Vector3d LogSO3(const Eigen::Matrix3d &rotation) {
  Eigen::Quaterniond q(rotation);
  q.normalize();
  if (q.w() < 0.0) {
    q.coeffs() *= -1.0;
  }
  const Eigen::Vector3d vector_part(q.x(), q.y(), q.z());
  const double sin_half_theta = vector_part.norm();
  if (sin_half_theta < 1e-12) {
    return 2.0 * vector_part;
  }
  const double theta = 2.0 * std::atan2(sin_half_theta, q.w());
  return (theta / sin_half_theta) * vector_part;
}

/** Exact Super right-Jacobian convention used by Predict. */
inline Eigen::Matrix3d RightJacobianSO3(const Eigen::Vector3d &angular_velocity,
                                        double dt) {
  const Eigen::Vector3d phi = angular_velocity * dt;
  const double theta = phi.norm();
  if (!phi.allFinite() || !std::isfinite(dt) || !std::isfinite(theta)) {
    return Eigen::Matrix3d::Identity();
  }
  const Eigen::Matrix3d K = HatSO3(phi);
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
  return Eigen::Matrix3d::Identity() - A * K + B * K * K;
}

enum class CovarianceValidity {
  kFiniteSymmetric,
  kNonFinite,
  kAsymmetric,
  kNegativeEigenvalue,
  kSingular,
};

CovarianceValidity ClassifyCovariance(const Matrix19 &covariance,
                                      double symmetry_tolerance = 1e-12,
                                      double singular_tolerance = 1e-12,
                                      double negative_tolerance = 1e-10);

/** Apply the canonical Super LIO retraction to the host-layout state. */
void ApplySuperLioIncrement19(StatesGroup &state, const Vector19 &increment,
                              double gravity_norm);

/** Difference in the canonical Super right-error coordinates, embedded in x19. */
Vector19 StateDifference19(const StatesGroup &predicted, const StatesGroup &current);

struct ImuSample {
  double timestamp = 0.0;
  Eigen::Vector3d acceleration = Eigen::Vector3d::Zero();
  Eigen::Vector3d angular_velocity = Eigen::Vector3d::Zero();
};

/** One authoritative Super-style snapshot used by scan undistortion. */
struct PropagationSnapshot {
  double timestamp = 0.0;
  Eigen::Matrix3d rotation = Eigen::Matrix3d::Identity();
  Eigen::Vector3d position = Eigen::Vector3d::Zero();
  Eigen::Vector3d velocity = Eigen::Vector3d::Zero();
  Eigen::Vector3d acceleration = Eigen::Vector3d::Zero();
  Eigen::Vector3d angular_velocity = Eigen::Vector3d::Zero();
};

struct Options {
  int num_iterations = 3;
  double quit_eps = 1e-6;

  // These values intentionally follow Super's BuildNoise convention: they
  // are covariance entries, not standard deviations to be squared again.
  double gyro_variance = 1e-5;
  double accelerometer_variance = 1e-2;
  double gyro_bias_variance = 1e-6;
  double accelerometer_bias_variance = 1e-4;

  double imu_scale = 1.0;
  double gravity_norm = G_m_s2;

  // Host-compatible exposure random walk: cov_inv_expo * dt^2 when enabled.
  // Disabled by default so a frozen exposure gives exact 18D Super parity.
  bool exposure_random_walk_enabled = false;
  double exposure_process_variance = 0.2;
};

/**
 * Shared-state 19D ESKF core.
 *
 * The filter keeps a reference to the caller-owned StatesGroup.  It does not
 * own a second nominal state or covariance; only IMU timing and diagnostics
 * are persistent members.  VIO remains outside this class and can continue to
 * use FAST's original visual operators.
 */
class ProbESKF19 {
 public:
  using ObservationCallback =
      std::function<void(const StatesGroup &, bool need_converge, Matrix6 &,
                         Vector6 &)>;

  explicit ProbESKF19(StatesGroup &state, Options options = Options());

  StatesGroup &state() { return state_; }
  const StatesGroup &state() const { return state_; }
  const Options &options() const { return options_; }

  void ResetImuHistory();
  void SeedImuHistory(const ImuSample &imu);
  void SetObservationWindow(double last_observation_time,
                           double current_observation_time);
  void SetImuScale(double imu_scale) { options_.imu_scale = imu_scale; }
  void SetGravityNorm(double gravity_norm) { options_.gravity_norm = gravity_norm; }

  // The first sample establishes the midpoint history.  Subsequent calls use
  // the exact Super trapezoidal/midpoint propagation semantics.
  bool Predict(const ImuSample &imu,
               PropagationSnapshot *accepted_snapshot = nullptr);

  bool UpdateObserve(const ObservationCallback &observation);

  double current_time() const { return current_time_; }
  double last_imu_time() const { return last_imu_time_; }
  bool has_imu_history() const { return has_last_imu_; }
  PropagationSnapshot CurrentPropagationSnapshot() const;
  double last_observation_time() const { return last_observation_time_; }
  int last_update_iterations() const { return last_update_iterations_; }
  bool need_converge() const { return need_converge_; }
  const Vector19 &last_increment() const { return last_increment_; }

  static Matrix12 NoiseCovariance(const Options &options);

 private:
  StatesGroup &state_;
  Options options_;

  bool has_last_imu_ = false;
  ImuSample last_imu_;
  double current_time_ = 0.0;
  double last_imu_time_ = -1.0;
  double last_observation_time_ = -std::numeric_limits<double>::infinity();
  double current_observation_time_ = std::numeric_limits<double>::infinity();
  Eigen::Vector3d last_global_acceleration_ = Eigen::Vector3d::Zero();
  Eigen::Vector3d last_angular_velocity_ = Eigen::Vector3d::Zero();

  int last_update_iterations_ = 0;
  bool need_converge_ = false;
  Vector19 last_increment_ = Vector19::Zero();
};

}  // namespace prob_livo

#endif  // PROB_LIVO_PROB_ESKF19_H_
