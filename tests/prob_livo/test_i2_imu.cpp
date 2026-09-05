#include "test_i2_support.h"

#include <cmath>

namespace prob_livo_test {

namespace {

void CheckInitializedState(TestContext &context,
                           const prob_livo_test_oracle::InitResult &expected,
                           const prob_livo::ProbImuAdapter &adapter,
                           const StatesGroup &actual, const std::string &name) {
  CheckVector3(context, expected.mean_gyro, adapter.mean_gyro(),
               name + ".mean_gyro", 1e-12);
  CheckVector3(context, expected.mean_acceleration,
               adapter.mean_acceleration(), name + ".mean_acceleration", 1e-12);
  context.Record(name + ".imu_scale",
                 adapter.imu_scale() - expected.imu_scale);
  context.Check(std::abs(adapter.imu_scale() - expected.imu_scale) < 1e-12,
                name + " accel scale mismatch");
  CheckVector3(context, expected.gravity, adapter.initial_gravity(),
               name + ".body_gravity", 1e-12);
  CheckMatrix3(context, expected.rotation, actual.rot_end,
               name + ".rotation", 1e-12);
  CheckVector3(context, expected.position, actual.pos_end,
               name + ".position", 1e-12);
  CheckVector3(context, expected.mean_gyro, actual.bias_g,
               name + ".gyro_bias", 1e-12);
  CheckVector3(context, Eigen::Vector3d::Zero(), actual.bias_a,
               name + ".accel_bias", 1e-12);
  CheckVector3(context, Eigen::Vector3d(0.0, 0.0, -9.7946), actual.gravity,
               name + ".aligned_gravity", 1e-12);
  CheckPhysicalCovariance(context, expected.covariance, actual.cov,
                          name + ".covariance", 1e-12);
  context.Check(adapter.propagation_trace().empty(),
                name + " initialized before propagation unexpectedly");
}

}  // namespace

int RunI2InitializationTests(TestContext &context) {
  struct Fixture {
    Eigen::Vector3d acceleration;
    Eigen::Vector3d angular_velocity;
    bool transform;
  };
  const std::vector<Fixture> fixtures = {
      {Eigen::Vector3d(0.0, 0.0, 9.7), Eigen::Vector3d(0.01, -0.02, 0.03),
       false},
      {Eigen::Vector3d(2.0, 1.0, 9.4), Eigen::Vector3d(0.1, -0.05, 0.02),
       false},
      {Eigen::Vector3d(0.3, -0.4, 19.4), Eigen::Vector3d(-0.2, 0.08, 0.04),
       true}};

  for (std::size_t fixture_index = 0; fixture_index < fixtures.size();
       ++fixture_index) {
    const double gravity_norm = 9.7946;
    prob_livo::ProbImuAdapter::Options adapter_options =
        AdapterOptions(gravity_norm);
    prob_livo_test_oracle::InitConfig oracle_config;
    oracle_config.gravity_norm = gravity_norm;
    if (fixtures[fixture_index].transform) {
      adapter_options.lidar_to_robot_yaw =
          Eigen::AngleAxisd(0.37, Eigen::Vector3d::UnitZ()).toRotationMatrix();
      adapter_options.robot_origin = Eigen::Vector3d(1.0, -2.0, 0.5);
      oracle_config.lidar_to_robot_yaw = adapter_options.lidar_to_robot_yaw;
      oracle_config.robot_origin = adapter_options.robot_origin;
    }
    prob_livo::ProbImuAdapter adapter(adapter_options);
    StatesGroup host;
    InitializeHostState(host);
    const double exposure_before = host.inv_expo_time;
    const double exposure_covariance_before =
        host.cov(Layout::kExpo, Layout::kExpo);
    prob_livo::ProbESKF19 filter(host, FilterOptions(gravity_norm));
    const auto samples = MakeImuSequence(
        100.0 + fixture_index, 50, fixtures[fixture_index].acceleration,
        fixtures[fixture_index].angular_velocity);
    const auto expected = prob_livo_test_oracle::Initialize(samples, oracle_config);
    LidarMeasureGroup packet = MakeEpoch(
        samples.front().timestamp, samples.back().timestamp,
        samples.front().timestamp, samples, {});

    const auto result = adapter.ProcessLioEpoch(
        packet, filter, prob_livo::SchedulerMode::kOnlyLio);
    const std::string name = "i2.init." + std::to_string(fixture_index);
    context.Check(result.success && result.initialized && !result.propagated,
                  name + " did not transition at the 50-sample boundary");
    context.Check(adapter.initialized() && adapter.initialization_samples() == 50,
                  name + " initialization sample lifecycle was wrong");
    CheckInitializedState(context, expected, adapter, host, name);
    context.Check(std::abs(host.inv_expo_time - exposure_before) < 1e-14 &&
                      std::abs(host.cov(Layout::kExpo, Layout::kExpo) -
                               exposure_covariance_before) < 1e-14,
                  name + " IMU initialization changed host exposure state");
    context.Check(std::abs(filter.current_time() - expected.timestamp) < 1e-12,
                  name + " initialization timestamp mismatch");
  }

  // Feed 49 + 1 samples through separate scheduler packets: transition must
  // occur exactly at sample 50, not at the first packet boundary.
  prob_livo::ProbImuAdapter adapter(AdapterOptions());
  StatesGroup host;
  InitializeHostState(host);
  prob_livo::ProbESKF19 filter(host, FilterOptions());
  const auto samples = MakeImuSequence(150.0, 50, Eigen::Vector3d(0, 0, 9.7),
                                       Eigen::Vector3d(0.01, 0.02, -0.01));
  std::vector<prob_livo::ImuSample> first(samples.begin(), samples.begin() + 49);
  std::vector<prob_livo::ImuSample> second(samples.begin() + 49, samples.end());
  LidarMeasureGroup first_packet =
      MakeEpoch(150.0, 150.48, 150.0, first, {});
  LidarMeasureGroup second_packet =
      MakeEpoch(150.49, 150.49, 150.49, second, {});
  const auto first_result = adapter.ProcessLioEpoch(
      first_packet, filter, prob_livo::SchedulerMode::kOnlyLio);
  const auto second_result = adapter.ProcessLioEpoch(
      second_packet, filter, prob_livo::SchedulerMode::kOnlyLio);
  context.Check(first_result.success && !first_result.initialized &&
                    second_result.success && second_result.initialized,
                "split initialization did not transition on sample 50");
  context.Check(std::abs(filter.current_time() - 150.49) < 1e-12,
                "split initialization timestamp did not use sample 50");

  prob_livo::ProbImuAdapter::Options fast_options;
  fast_options.minimum_initialization_samples = 3;
  fast_options.gravity_norm = G_m_s2;
  fast_options.initialization_semantics =
      prob_livo::ImuInitializationSemantics::kFastLivo2Native;
  prob_livo::ProbImuAdapter fast_adapter(fast_options);
  StatesGroup fast_host;
  const auto prior = fast_host.cov;
  prob_livo::Options fast_filter_options;
  fast_filter_options.gravity_norm = G_m_s2;
  prob_livo::ProbESKF19 fast_filter(fast_host, fast_filter_options);
  const auto fast_samples = std::vector<prob_livo::ImuSample>{
      HostImu(400.00, Eigen::Vector3d(0.8, 0.1, -9.6),
              Eigen::Vector3d(0.01, 0.02, 0.03)),
      HostImu(400.01, Eigen::Vector3d(1.0, -0.1, -9.4),
              Eigen::Vector3d(0.02, 0.01, 0.04)),
      HostImu(400.02, Eigen::Vector3d(0.9, 0.0, -9.5),
              Eigen::Vector3d(0.03, 0.00, 0.02))};
  LidarMeasureGroup fast_packet =
      MakeEpoch(400.0, 400.02, 400.0, fast_samples, {});
  const auto fast_result = fast_adapter.ProcessLioEpoch(
      fast_packet, fast_filter, prob_livo::SchedulerMode::kOnlyLio);
  Eigen::Vector3d native_mean_acc = fast_samples.front().acceleration;
  Eigen::Vector3d native_mean_gyro = fast_samples.front().angular_velocity;
  int native_n = 1;
  for (const auto &sample : fast_samples) {
    native_mean_acc += (sample.acceleration - native_mean_acc) / native_n;
    native_mean_gyro +=
        (sample.angular_velocity - native_mean_gyro) / native_n;
    ++native_n;
  }
  const Eigen::Vector3d native_gravity =
      -native_mean_acc / native_mean_acc.norm() * G_m_s2;
  context.Check(fast_result.success && fast_result.initialized,
                "FAST-native initialization did not complete");
  CheckVector3(context, native_mean_acc, fast_adapter.mean_acceleration(),
               "p15.fast_init.mean_acc", 1e-15);
  CheckVector3(context, native_mean_gyro, fast_adapter.mean_gyro(),
               "p15.fast_init.mean_gyro", 1e-15);
  CheckMatrix3(context, Eigen::Matrix3d::Identity(), fast_host.rot_end,
               "p15.fast_init.rotation", 1e-15);
  CheckVector3(context, Eigen::Vector3d::Zero(), fast_host.pos_end,
               "p15.fast_init.position", 1e-15);
  CheckVector3(context, Eigen::Vector3d::Zero(), fast_host.vel_end,
               "p15.fast_init.velocity", 1e-15);
  CheckVector3(context, Eigen::Vector3d::Zero(), fast_host.bias_g,
               "p15.fast_init.gyro_bias", 1e-15);
  CheckVector3(context, Eigen::Vector3d::Zero(), fast_host.bias_a,
               "p15.fast_init.accel_bias", 1e-15);
  CheckVector3(context, native_gravity, fast_host.gravity,
               "p15.fast_init.gravity", 1e-15);
  context.Check((fast_host.cov - prior).cwiseAbs().maxCoeff() == 0.0,
                "FAST-native initialization replaced the host prior");
  context.Check(fast_adapter.initialization_samples() ==
                        static_cast<int>(fast_samples.size()) &&
                    fast_adapter.initialization_window_start() == 400.00 &&
                    fast_adapter.initialization_window_end() == 400.02,
                "FAST-native initialization window accounting mismatch");
  return context.Passed() ? 0 : 1;
}

int RunI2PropagationTests(TestContext &context) {
  const double gravity_norm = 9.7946;
  prob_livo::ProbImuAdapter adapter(AdapterOptions(gravity_norm));
  StatesGroup host;
  InitializeHostState(host);
  prob_livo::ProbESKF19 filter(host, FilterOptions(gravity_norm));
  const auto init_samples = MakeImuSequence(
      200.0, 50, Eigen::Vector3d(0.2, -0.3, 9.5),
      Eigen::Vector3d(0.03, -0.02, 0.04));
  LidarMeasureGroup init_packet =
      MakeEpoch(200.0, 200.49, 200.0, init_samples, {});
  const auto init_result = adapter.ProcessLioEpoch(
      init_packet, filter, prob_livo::SchedulerMode::kOnlyLio);
  context.Check(init_result.success && init_result.initialized,
                "propagation fixture failed initialization");

  const auto propagation_samples = std::vector<prob_livo::ImuSample>{
      HostImu(200.50, Eigen::Vector3d(0.4, -0.1, 9.3),
              Eigen::Vector3d(0.10, 0.03, -0.02)),
      HostImu(200.51, Eigen::Vector3d(0.5, 0.0, 9.1),
              Eigen::Vector3d(0.12, 0.02, -0.01)),
      HostImu(200.52, Eigen::Vector3d(0.7, 0.2, 8.9),
              Eigen::Vector3d(0.15, 0.01, 0.00))};
  LidarMeasureGroup packet = MakeEpoch(
      200.49, 200.52, 200.49, propagation_samples,
      {{200.49, 1}, {200.505, 2}, {200.52, 3}});
  const auto result = adapter.ProcessLioEpoch(
      packet, filter, prob_livo::SchedulerMode::kOnlyLio);
  context.Check(result.success && result.propagated,
                "exact-end propagation epoch failed");
  context.Check(std::abs(filter.current_time() - 200.52) < 1e-12,
                "exact-end propagation stopped at the wrong time");

  prob_livo_test_oracle::Options oracle_options;
  oracle_options.gravity_norm = gravity_norm;
  oracle_options.imu_scale = adapter.imu_scale();
  prob_livo_test_oracle::InitConfig init_config;
  init_config.gravity_norm = gravity_norm;
  const auto init_expected =
      prob_livo_test_oracle::Initialize(init_samples, init_config);
  prob_livo_test_oracle::Propagation oracle(oracle_options);
  SetOracleFromInit(oracle.state, init_expected);
  oracle.current_time = init_expected.timestamp;
  oracle.last_observation_time = 200.49;
  oracle.current_observation_time = 200.52;
  oracle.Seed(init_samples.back());
  std::vector<prob_livo_test_oracle::DynamicSnapshot> expected_trace;
  expected_trace.push_back(oracle.Snapshot());
  for (const auto &sample : propagation_samples) {
    if (oracle.Predict(sample)) expected_trace.push_back(oracle.Snapshot());
  }
  const auto &actual_trace = adapter.propagation_trace();
  context.Check(actual_trace.size() == expected_trace.size(),
                "propagation trace sample count differs from Super");
  const std::size_t trace_count =
      std::min(actual_trace.size(), expected_trace.size());
  double max_trace_error = 0.0;
  for (std::size_t index = 0; index < trace_count; ++index) {
    const auto &expected_snapshot = expected_trace[index];
    const auto &actual_snapshot = actual_trace[index];
    max_trace_error = std::max(
        max_trace_error,
        std::abs(expected_snapshot.timestamp - actual_snapshot.timestamp));
    max_trace_error = std::max(
        max_trace_error,
        (expected_snapshot.rotation - actual_snapshot.rotation).cwiseAbs().maxCoeff());
    max_trace_error = std::max(
        max_trace_error,
        (expected_snapshot.position - actual_snapshot.position).norm());
    max_trace_error = std::max(
        max_trace_error,
        (expected_snapshot.velocity - actual_snapshot.velocity).norm());
    max_trace_error = std::max(
        max_trace_error,
        (expected_snapshot.acceleration - actual_snapshot.acceleration).norm());
    max_trace_error = std::max(
        max_trace_error,
        (expected_snapshot.angular_velocity -
         actual_snapshot.angular_velocity)
            .norm());
  }
  context.Record("i2.propagation.max_trace_error", max_trace_error);
  context.Check(max_trace_error < 2e-11,
                "Super propagation trace differs from ProbESKF19 trace");
  CheckState(context, oracle.state, host, "i2.propagation.final_state", 2e-11);
  CheckPhysicalCovariance(context, oracle.state.covariance, host.cov,
                          "i2.propagation.final_covariance", 2e-10);
  context.Check(std::abs(packet.last_lio_update_time - 200.49) < 1e-12,
                "adapter incorrectly owned the scheduler anchor");

  // Endpoint between samples: no-lookahead is rejected before propagation;
  // the explicit non-consuming look-ahead then produces the clipped endpoint.
  prob_livo::ProbImuAdapter partial_adapter(AdapterOptions(gravity_norm));
  StatesGroup partial_host;
  InitializeHostState(partial_host);
  prob_livo::ProbESKF19 partial_filter(partial_host, FilterOptions(gravity_norm));
  LidarMeasureGroup partial_init = MakeEpoch(
      300.0, 300.49, 300.0,
      MakeImuSequence(300.0, 50, Eigen::Vector3d(0, 0, 9.7),
                      Eigen::Vector3d::Zero()),
      {});
  partial_adapter.ProcessLioEpoch(partial_init, partial_filter,
                                  prob_livo::SchedulerMode::kOnlyLio);
  const auto partial_samples = std::vector<prob_livo::ImuSample>{
      HostImu(300.50, Eigen::Vector3d(0, 0, 9.7),
              Eigen::Vector3d(0.1, 0.0, 0.0)),
      HostImu(300.51, Eigen::Vector3d(0, 0, 9.7),
              Eigen::Vector3d(0.1, 0.0, 0.0))};
  LidarMeasureGroup partial_packet = MakeEpoch(
      300.49, 300.515, 300.49, partial_samples, {{300.515, 7}});
  const double previous_anchor = partial_packet.last_lio_update_time;
  const double previous_filter_time = partial_filter.current_time();
  const auto rejected = partial_adapter.ProcessLioEpoch(
      partial_packet, partial_filter, prob_livo::SchedulerMode::kOnlyLio);
  context.Check(!rejected.success &&
                    partial_packet.last_lio_update_time == previous_anchor &&
                    partial_filter.current_time() == previous_filter_time,
                "missing endpoint look-ahead was not rejected transactionally");
  const prob_livo::ImuSample lookahead =
      HostImu(300.52, Eigen::Vector3d(0, 0, 9.7),
              Eigen::Vector3d(0.1, 0.0, 0.0));
  const auto accepted = partial_adapter.ProcessLioEpoch(
      partial_packet, partial_filter, prob_livo::SchedulerMode::kOnlyLio,
      &lookahead);
  context.Check(accepted.success && accepted.propagated &&
                    std::abs(partial_filter.current_time() - 300.515) < 1e-12,
                "partial endpoint look-ahead did not clip to epoch end");
  context.Check(std::abs(partial_filter.last_imu_time() - 300.52) < 1e-12,
                "partial endpoint did not retain Super look-ahead history");
  context.Check(std::abs(partial_packet.last_lio_update_time - 300.49) < 1e-12,
                "partial endpoint mutated scheduler anchor");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
