#include "test_i3_support.h"

#include <cmath>

namespace prob_livo_test {

int RunP4MapTimingTests(TestContext &context) {
  StatesGroup state;
  auto options = TestBackendOptions();
  prob_livo::ProbLioBackend backend(state, options);
  const auto points = MakePlanePoints();

  for (int index = 0; index < 54; ++index) {
    const double start = 0.01 * index;
    const auto imu = std::vector<prob_livo::ImuSample>{
        {start + 0.005, Eigen::Vector3d(0.0, 0.0, 9.7946),
         Eigen::Vector3d::Zero()}};
    LidarMeasureGroup packet = MakeBackendEpoch(start, start + 0.01, imu,
                                                points);
    context.Check(backend.ProcessEpoch(packet),
                  "map-init timing fixture rejected an epoch");
  }

  context.Check(backend.lifecycle_state() == prob_livo::ProbLioLifecycle::RUN,
                "map-init timing fixture did not reach RUN");
  context.Check(backend.counters().imu_init_epochs == 50 &&
                    backend.counters().map_init_epochs == 4,
                "map-init timing count diverged from Super lifecycle");
  context.Check(std::abs(backend.last_observation_time() - 0.54) < 1e-12,
                "map_init did not advance the observation boundary");
  context.Check(std::abs(backend.filter_current_time() - 0.495) < 1e-12,
                "map_init unexpectedly propagated the filter");

  const auto run_imu = std::vector<prob_livo::ImuSample>{
      {0.545, Eigen::Vector3d(0.0, 0.0, 9.7946), Eigen::Vector3d::Zero()}};
  const auto lookahead = ToRosImu(prob_livo::ImuSample{
      0.555, Eigen::Vector3d(0.0, 0.0, 9.7946), Eigen::Vector3d::Zero()});
  LidarMeasureGroup run_packet = MakeBackendEpoch(
      0.54, 0.55, run_imu, points, lookahead);
  // Exercise both sides of the Super source-order boundary with finite point
  // times: the first point is after the endpoint, while the second predates
  // the propagation trace front because the scan windows overlap.
  run_packet.lidar_frame_beg_time = 0.48;
  // Super Ouster source order can place a point after the endpoint defined by
  // the final accepted sampled point. The legacy undistorter keeps that point
  // in the raw LiDAR->IMU frame instead of rejecting the whole epoch.
  run_packet.pcl_proc_cur->points.front().curvature = 80.0f;
  context.Check(backend.ProcessEpoch(run_packet),
                "first RUN timing bridge rejected the epoch");
  context.Check(std::abs(backend.filter_current_time() - 0.55) < 1e-12,
                "first RUN epoch did not reach its endpoint");
  context.Check(std::abs(backend.last_observation_time() - 0.55) < 1e-12,
                "first RUN observation boundary did not advance");
  context.Check(backend.undistorted_scan()->size() == points.size(),
                "first RUN undistorted cloud size mismatch");
  context.Check(std::abs(backend.undistorted_scan()->points.front().x -
                         points.front().x) < 1e-6f &&
                    std::abs(backend.undistorted_scan()->points.front().y -
                             points.front().y) < 1e-6f &&
                    std::abs(backend.undistorted_scan()->points.front().z -
                             points.front().z) < 1e-6f,
                "post-endpoint legacy point was not kept in raw IMU frame");
  context.Check(state.cov.allFinite(),
                "first RUN covariance is not finite");
  context.Record("G-P4.4 first_run_time", backend.filter_current_time());

  // Legacy Super does not consume the first IMU after the scan endpoint as a
  // look-ahead.  Keep this mode-specific seam covered without changing the
  // FAST-native endpoint contract above.
  StatesGroup legacy_state;
  auto legacy_options = TestBackendOptions();
  legacy_options.legacy_super_timing = true;
  prob_livo::ProbLioBackend legacy_backend(legacy_state, legacy_options);
  for (int index = 0; index < 54; ++index) {
    const double start = 0.01 * index;
    const auto imu = std::vector<prob_livo::ImuSample>{
        {start + 0.005, Eigen::Vector3d(0.0, 0.0, 9.7946),
         Eigen::Vector3d::Zero()}};
    LidarMeasureGroup packet =
        MakeBackendEpoch(start, start + 0.01, imu, points);
    context.Check(legacy_backend.ProcessEpoch(packet),
                  "legacy timing fixture rejected an init/map epoch");
  }
  const auto legacy_run_imu = std::vector<prob_livo::ImuSample>{
      {0.545, Eigen::Vector3d(0.0, 0.0, 9.7946), Eigen::Vector3d::Zero()}};
  LidarMeasureGroup legacy_run_packet =
      MakeBackendEpoch(0.54, 0.55, legacy_run_imu, points);
  context.Check(legacy_backend.ProcessEpoch(legacy_run_packet),
                "legacy timing fixture rejected a partial-endpoint RUN epoch");
  context.Check(std::abs(legacy_backend.filter_current_time() - 0.545) < 1e-12,
                "legacy timing unexpectedly consumed endpoint look-ahead");
  context.Check(std::abs(legacy_backend.last_observation_time() - 0.55) <
                    1e-12,
                "legacy timing did not advance observation boundary");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
