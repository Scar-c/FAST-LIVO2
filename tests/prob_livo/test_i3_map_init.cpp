#include "test_i3_support.h"

namespace prob_livo_test {

int RunI3MapInitTests(TestContext &context) {
  StatesGroup state;
  prob_livo::ProbLioBackend backend(state, TestBackendOptions());
  const auto points = MakePlanePoints();
  for (int index = 0; index < 54; ++index) {
    const double start = 0.01 * index;
    const double end = start + 0.01;
    const auto imu = std::vector<prob_livo::ImuSample>{
        {start + 0.005, Eigen::Vector3d(0.0, 0.0, 9.7946),
         Eigen::Vector3d::Zero()}};
    LidarMeasureGroup packet = MakeBackendEpoch(start, end, imu, points);
    context.Check(backend.ProcessEpoch(packet),
                  "backend rejected an IMU_INIT/MAP_INIT epoch");
  }
  context.Check(backend.lifecycle_state() == prob_livo::ProbLioLifecycle::RUN,
                "backend did not complete four-scan map initialization");
  context.Check(backend.counters().imu_init_epochs == 50 &&
                    backend.counters().map_init_epochs == 4,
                "backend lifecycle counters do not match 50+4 contract");
  context.Check(backend.counters().map_init_inserts == points.size() * 4,
                "map-init did not insert each raw scan exactly once");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
