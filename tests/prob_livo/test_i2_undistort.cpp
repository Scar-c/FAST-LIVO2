#include "test_i2_support.h"

#include <Eigen/Geometry>

#include <cmath>

namespace prob_livo_test {

int RunI2UndistortionTests(TestContext &context) {
  struct MotionFixture {
    Eigen::Vector3d acceleration;
    Eigen::Vector3d angular_velocity;
  };
  const std::vector<MotionFixture> fixtures = {
      {Eigen::Vector3d(0.0, 0.0, 9.7), Eigen::Vector3d::Zero()},
      {Eigen::Vector3d(1.0, 0.0, 9.7), Eigen::Vector3d::Zero()},
      {Eigen::Vector3d(0.0, 0.0, 9.7), Eigen::Vector3d(0.0, 0.0, 0.5)},
      {Eigen::Vector3d(0.5, -0.2, 9.7), Eigen::Vector3d(0.1, 0.2, -0.15)}};

  prob_livo::ProbImuAdapter::Options adapter_options = AdapterOptions();
  adapter_options.lidar_to_imu_rotation =
      Eigen::AngleAxisd(0.27, Eigen::Vector3d::UnitY()).toRotationMatrix();
  adapter_options.lidar_to_imu_translation = Eigen::Vector3d(0.12, -0.07, 0.2);
  for (std::size_t fixture_index = 0; fixture_index < fixtures.size();
       ++fixture_index) {
    const double base = 400.0 + 10.0 * fixture_index;
    prob_livo::ProbImuAdapter adapter(adapter_options);
    StatesGroup host;
    InitializeHostState(host);
    prob_livo::ProbESKF19 filter(host, FilterOptions());
    const auto init_samples = MakeImuSequence(
        base, 50, Eigen::Vector3d(0.0, 0.0, 9.7),
        Eigen::Vector3d::Zero());
    LidarMeasureGroup init_packet =
        MakeEpoch(base, base + 0.49, base, init_samples, {});
    const auto init_result = adapter.ProcessLioEpoch(
        init_packet, filter, prob_livo::SchedulerMode::kOnlyLio);
    context.Check(init_result.success && init_result.initialized,
                  "undistortion fixture failed initialization");

    const std::vector<prob_livo::ImuSample> imu_samples = {
        HostImu(base + 0.50, fixtures[fixture_index].acceleration,
                fixtures[fixture_index].angular_velocity),
        HostImu(base + 0.55, fixtures[fixture_index].acceleration,
                fixtures[fixture_index].angular_velocity),
        HostImu(base + 0.59, fixtures[fixture_index].acceleration,
                fixtures[fixture_index].angular_velocity)};
    const std::vector<std::pair<double, int>> point_specs = {
        {base + 0.49, 1}, {base + 0.515, 2}, {base + 0.55, 3},
        {base + 0.59, 4}};
    LidarMeasureGroup packet = MakeEpoch(
        base + 0.49, base + 0.59, base + 0.49, imu_samples, point_specs);
    const auto result = adapter.ProcessLioEpoch(
        packet, filter, prob_livo::SchedulerMode::kOnlyLio);
    const std::string name = "i2.undistort." + std::to_string(fixture_index);
    context.Check(result.success && result.propagated,
                  name + " did not produce a scan-end IMU cloud");
    context.Check(result.prob_scan_undistort_imu->size() == point_specs.size(),
                  name + " changed point count");

    prob_livo_test_oracle::Options oracle_options;
    oracle_options.gravity_norm = 9.7946;
    oracle_options.imu_scale = adapter.imu_scale();
    prob_livo_test_oracle::InitConfig init_config;
    init_config.gravity_norm = 9.7946;
    const auto init_expected =
        prob_livo_test_oracle::Initialize(init_samples, init_config);
    prob_livo_test_oracle::Propagation oracle(oracle_options);
    SetOracleFromInit(oracle.state, init_expected);
    oracle.Seed(init_samples.back());
    oracle.last_observation_time = base + 0.49;
    oracle.current_observation_time = base + 0.59;
    std::vector<prob_livo_test_oracle::DynamicSnapshot> expected_trace;
    expected_trace.push_back(oracle.Snapshot());
    for (const auto &sample : imu_samples) {
      if (oracle.Predict(sample)) expected_trace.push_back(oracle.Snapshot());
    }

    double max_xyz_error = 0.0;
    for (std::size_t index = 0; index < point_specs.size(); ++index) {
      const PointType &input = packet.pcl_proc_cur->points[index];
      const PointType &actual = result.prob_scan_undistort_imu->points[index];
      const Eigen::Vector3d expected = prob_livo_test_oracle::UndistortPoint(
          Eigen::Vector3d(input.x, input.y, input.z),
          base + 0.49 + static_cast<double>(input.curvature) / 1000.0,
          expected_trace, adapter_options.lidar_to_imu_rotation,
          adapter_options.lidar_to_imu_translation);
      const double xyz_error =
          (expected - Eigen::Vector3d(actual.x, actual.y, actual.z)).norm();
      max_xyz_error = std::max(max_xyz_error, xyz_error);
      context.Check(std::abs(actual.intensity - input.intensity) < 1e-6,
                    name + " changed intensity identity");
      context.Check(std::abs(actual.curvature - input.curvature) < 1e-6,
                    name + " changed point time provenance");
      context.Check(std::abs(actual.normal_x - input.normal_x) < 1e-6 &&
                        std::abs(actual.normal_y - input.normal_y) < 1e-6 &&
                        std::abs(actual.normal_z - input.normal_z) < 1e-6,
                    name + " changed non-geometric point fields");
    }
    context.Record(name + ".max_xyz_error", max_xyz_error);
    context.Check(max_xyz_error < 2e-6,
                  name + " diverged from Super slerp/translation undistortion");

    if (fixture_index == 0) {
      const PointType &raw = packet.pcl_proc_cur->points.front();
      const PointType &actual = result.prob_scan_undistort_imu->points.front();
      const Eigen::Vector3d raw_vector(raw.x, raw.y, raw.z);
      const Eigen::Vector3d expected_extrinsic =
          adapter_options.lidar_to_imu_rotation * raw_vector +
          adapter_options.lidar_to_imu_translation;
      context.Check((expected_extrinsic -
                     Eigen::Vector3d(actual.x, actual.y, actual.z))
                        .norm() < 2e-6,
                    "scan-end output was not expressed in the IMU frame");
      context.Check((raw_vector - expected_extrinsic).norm() > 1e-2,
                    "LiDAR-frame output mutation was not discriminated");
      const Eigen::Vector3d no_translation =
          adapter_options.lidar_to_imu_rotation * raw_vector;
      context.Check((no_translation - expected_extrinsic).norm() > 1e-2,
                    "omitted extrinsic translation mutation was not discriminated");
    }
  }
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
