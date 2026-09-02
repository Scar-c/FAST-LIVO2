#include "test_i3_support.h"

namespace prob_livo_test {

int RunI3BackendSeamTests(TestContext &context) {
  StatesGroup state;
  auto options = TestBackendOptions();
  prob_livo::ProbLioBackend backend(state, options);
  const auto points = MakePlanePoints();
  for (int index = 0; index < 54; ++index) {
    const double start = 0.01 * index;
    const auto imu = std::vector<prob_livo::ImuSample>{
        {start + 0.005, Eigen::Vector3d(0.0, 0.0, 9.7946),
         Eigen::Vector3d::Zero()}};
    LidarMeasureGroup packet =
        MakeBackendEpoch(start, start + 0.01, imu, points);
    backend.ProcessEpoch(packet);
  }
  auto run_imu = std::vector<prob_livo::ImuSample>{
      {0.545, Eigen::Vector3d(0.0, 0.0, 9.7946), Eigen::Vector3d::Zero()}};
  sensor_msgs::Imu::ConstPtr lookahead = ToRosImu(
      prob_livo::ImuSample{0.555, Eigen::Vector3d(0.0, 0.0, 9.7946),
                           Eigen::Vector3d::Zero()});
  LidarMeasureGroup packet =
      MakeBackendEpoch(0.54, 0.55, run_imu, points, lookahead);
  context.Check(backend.lifecycle_state() == prob_livo::ProbLioLifecycle::RUN,
                "backend seam test did not reach RUN");
  context.Check(backend.ProcessEpoch(packet),
                "backend did not execute the camera-OFF RUN seam");
  context.Check(backend.counters().run_epochs == 1 &&
                    backend.counters().undistorted_points > 0 &&
                    backend.counters().downsampled_points > 0,
                "RUN seam did not execute undistort/downsample stages");
  context.Check(backend.counters().hknn_queries > 0,
                "RUN seam did not query the Super HKNN map");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
