#include "test_i2_support.h"

#include <Eigen/Geometry>

#include <cmath>
#include <vector>

namespace prob_livo_test {

namespace {

struct LatePointFixture {
  double origin = 0.0;
  double endpoint = 0.0;
  std::vector<std::pair<double, int>> point_specs;
};

LatePointFixture MakeLatePointFixture(double base) {
  LatePointFixture fixture;
  fixture.origin = base + 0.49;
  fixture.endpoint = base + 0.59;
  // The final source-order point is not the maximum-time point.  The two
  // late points reproduce the Oxford magnitudes observed in Prompt16.
  fixture.point_specs = {{base + 0.50, 1},
                         {base + 0.59, 2},
                         {base + 0.5900002, 3},
                         {base + 0.5900014, 4},
                         {base + 0.55, 5}};
  return fixture;
}

LidarMeasureGroup MakeInitializedPacket(
    double base, const std::vector<prob_livo::ImuSample> &init_samples) {
  return MakeEpoch(base, base + 0.49, base, init_samples, {});
}

Eigen::Vector3d ExpectedLatePoint(
    const PointType &input, double query_time,
    const prob_livo::PropagationSnapshot &terminal,
    const Eigen::Matrix3d &lidar_to_imu_rotation,
    const Eigen::Vector3d &lidar_to_imu_translation) {
  const double dt = query_time - terminal.timestamp;
  const Eigen::Matrix3d point_rotation =
      terminal.rotation *
      prob_livo::ExpSO3(terminal.angular_velocity * dt);
  const Eigen::Vector3d point_position =
      terminal.position + terminal.velocity * dt +
      0.5 * terminal.acceleration * dt * dt;
  const Eigen::Vector3d lidar_in_imu =
      lidar_to_imu_rotation * Eigen::Vector3d(input.x, input.y, input.z) +
      lidar_to_imu_translation;
  return terminal.rotation.transpose() *
         (point_rotation * lidar_in_imu + point_position - terminal.position);
}

double StateMaxDelta(const StatesGroup &first, const StatesGroup &second) {
  double delta = (first.rot_end - second.rot_end).cwiseAbs().maxCoeff();
  delta = std::max(delta, (first.pos_end - second.pos_end).cwiseAbs().maxCoeff());
  delta = std::max(delta, (first.vel_end - second.vel_end).cwiseAbs().maxCoeff());
  delta = std::max(delta, (first.bias_g - second.bias_g).cwiseAbs().maxCoeff());
  delta = std::max(delta, (first.bias_a - second.bias_a).cwiseAbs().maxCoeff());
  delta = std::max(delta, (first.gravity - second.gravity).cwiseAbs().maxCoeff());
  delta = std::max(delta, std::abs(first.inv_expo_time - second.inv_expo_time));
  delta = std::max(delta, (first.cov - second.cov).cwiseAbs().maxCoeff());
  return delta;
}

}  // namespace

int RunI2LatePointTests(TestContext &context) {
  const double base = 800.0;
  const LatePointFixture fixture = MakeLatePointFixture(base);
  prob_livo::ProbImuAdapter::Options adapter_options = AdapterOptions();
  adapter_options.lidar_to_imu_rotation =
      Eigen::AngleAxisd(0.31, Eigen::Vector3d(0.3, -0.4, 0.5).normalized())
          .toRotationMatrix();
  adapter_options.lidar_to_imu_translation =
      Eigen::Vector3d(0.27, -0.19, 0.13);
  context.Check(std::abs(adapter_options.time_tolerance - 2e-8) < 1e-20,
                "late-point fixture changed the 20 ns tolerance contract");

  const auto init_samples = MakeImuSequence(
      base, 50, Eigen::Vector3d(0.7, -0.3, 9.7),
      Eigen::Vector3d(0.02, -0.04, 0.03));
  const std::vector<prob_livo::ImuSample> imu_samples = {
      HostImu(base + 0.50, Eigen::Vector3d(1.4, -0.8, 9.5),
              Eigen::Vector3d(1.1, -0.7, 1.8)),
      HostImu(base + 0.55, Eigen::Vector3d(1.4, -0.8, 9.5),
              Eigen::Vector3d(1.1, -0.7, 1.8)),
      HostImu(base + 0.59, Eigen::Vector3d(1.4, -0.8, 9.5),
              Eigen::Vector3d(1.1, -0.7, 1.8))};

  // A is the no-late-point reference.  B has the same epoch and IMU packet,
  // but includes late points.  The terminal filter state and covariance must
  // remain identical between A and B.
  prob_livo::ProbImuAdapter adapter_a(adapter_options);
  prob_livo::ProbImuAdapter adapter_b(adapter_options);
  StatesGroup state_a;
  StatesGroup state_b;
  InitializeHostState(state_a);
  state_b = state_a;
  prob_livo::ProbESKF19 filter_a(state_a, FilterOptions());
  prob_livo::ProbESKF19 filter_b(state_b, FilterOptions());
  LidarMeasureGroup init_packet_a =
      MakeInitializedPacket(base, init_samples);
  LidarMeasureGroup init_packet_b =
      MakeInitializedPacket(base, init_samples);
  const auto init_a = adapter_a.ProcessLioEpoch(
      init_packet_a, filter_a,
      prob_livo::SchedulerMode::kOnlyLio);
  const auto init_b = adapter_b.ProcessLioEpoch(
      init_packet_b, filter_b,
      prob_livo::SchedulerMode::kOnlyLio);
  context.Check(init_a.success && init_a.initialized && init_b.success &&
                    init_b.initialized,
                "late-point fixture failed initialization");

  const std::vector<std::pair<double, int>> no_late_points = {
      {base + 0.50, 1}, {base + 0.55, 2}, {base + 0.58, 3},
      {base + 0.59, 4}};
  LidarMeasureGroup packet_a =
      MakeEpoch(fixture.origin, fixture.endpoint, fixture.origin, imu_samples,
                no_late_points);
  LidarMeasureGroup packet_b =
      MakeEpoch(fixture.origin, fixture.endpoint, fixture.origin, imu_samples,
                fixture.point_specs);
  const auto result_a = adapter_a.ProcessLioEpoch(
      packet_a, filter_a, prob_livo::SchedulerMode::kOnlyLio);
  const auto result_b = adapter_b.ProcessLioEpoch(
      packet_b, filter_b, prob_livo::SchedulerMode::kOnlyLio);

  context.Check(result_a.success && result_a.propagated,
                "no-late reference epoch did not propagate");
  // This is the red-capable assertion: the pre-fix implementation rejects B
  // before Undistort because one point is after the scheduler endpoint.
  context.Check(result_b.success && result_b.propagated,
                "late point rejected the complete LIO epoch");
  context.Check(result_b.prob_scan_undistort_imu->size() ==
                    fixture.point_specs.size(),
                "late-point correction changed scan cardinality");
  context.Check(std::abs(packet_b.last_lio_update_time - fixture.origin) <
                    1e-12,
                "late-point processing changed lifecycle epoch anchor");

  const double state_delta = StateMaxDelta(state_a, state_b);
  context.Record("i2.late_point.state_covariance_delta", state_delta);
  context.Check(state_delta < 1e-12,
                "late-point handling mutated shared state or covariance");
  const double time_delta = std::abs(filter_a.current_time() -
                                     filter_b.current_time());
  context.Record("i2.late_point.timestamp_delta", time_delta);
  context.Check(time_delta < 1e-12,
                "late-point handling changed shared filter timestamp");

  // LIVO uses a different point-time origin at the scheduler seam, but it
  // must reach the same shared Undistort implementation and late semantics.
  prob_livo::ProbImuAdapter adapter_livo(adapter_options);
  StatesGroup state_livo;
  InitializeHostState(state_livo);
  prob_livo::ProbESKF19 filter_livo(state_livo, FilterOptions());
  LidarMeasureGroup init_packet_livo =
      MakeInitializedPacket(base, init_samples);
  const auto init_livo = adapter_livo.ProcessLioEpoch(
      init_packet_livo, filter_livo, prob_livo::SchedulerMode::kLivo);
  LidarMeasureGroup packet_livo =
      MakeEpoch(fixture.origin, fixture.endpoint, fixture.origin, imu_samples,
                fixture.point_specs);
  const auto result_livo = adapter_livo.ProcessLioEpoch(
      packet_livo, filter_livo, prob_livo::SchedulerMode::kLivo);
  context.Check(init_livo.success && init_livo.initialized &&
                    result_livo.success && result_livo.propagated,
                "LIVO scheduler did not use shared late-point semantics");
  context.Check(result_livo.prob_scan_undistort_imu->size() ==
                    fixture.point_specs.size(),
                "LIVO late-point correction changed scan cardinality");
  context.Check(StateMaxDelta(state_a, state_livo) < 1e-12,
                "LIVO late-point path changed shared state or covariance");

  if (!result_b.success ||
      result_b.prob_scan_undistort_imu->size() != fixture.point_specs.size()) {
    return 0;
  }

  const auto terminal = filter_a.CurrentPropagationSnapshot();
  for (std::size_t index = 0; index < fixture.point_specs.size(); ++index) {
    const PointType &input = packet_b.pcl_proc_cur->points[index];
    const PointType &actual = result_b.prob_scan_undistort_imu->points[index];
    const double query_time = fixture.origin +
                              static_cast<double>(input.curvature) / 1000.0;
    const double lateness = query_time - fixture.endpoint;
    if (index == 2 || index == 3) {
      context.Record("i2.late_point.lateness_us." + std::to_string(index),
                     lateness * 1e6);
      context.Check(lateness > (index == 2 ? 0.1e-6 : 1.0e-6) &&
                        lateness < (index == 2 ? 0.3e-6 : 1.8e-6),
                    "late-point fixture did not preserve the target lateness");
      const Eigen::Vector3d expected = ExpectedLatePoint(
          input, query_time, terminal, adapter_options.lidar_to_imu_rotation,
          adapter_options.lidar_to_imu_translation);
      const double error =
          (expected - Eigen::Vector3d(actual.x, actual.y, actual.z)).norm();
      context.Record("i2.late_point.expected_error." + std::to_string(index),
                     error);
      context.Check(error < 3e-6,
                    "late point did not use terminal-motion extrapolation");

      const Eigen::Vector3d raw(input.x, input.y, input.z);
      const Eigen::Vector3d lidar_in_imu =
          adapter_options.lidar_to_imu_rotation * raw +
          adapter_options.lidar_to_imu_translation;
      const double fallback_error =
          (lidar_in_imu - Eigen::Vector3d(actual.x, actual.y, actual.z)).norm();
      context.Record("i2.late_point.fallback_discrimination." +
                         std::to_string(index),
                     fallback_error);
      context.Check(fallback_error > 1e-7,
                    "late point matched endpoint-clamp/Super fallback mutation");
    }
    context.Check(std::abs(actual.intensity - input.intensity) < 1e-6,
                  "late-point handling changed intensity identity");
    context.Check(std::abs(actual.curvature - input.curvature) < 1e-6,
                  "late-point handling changed time provenance");
  }

  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
