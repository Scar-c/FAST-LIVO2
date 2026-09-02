#include "test_i2_support.h"

#include <fstream>
#include <iterator>

namespace prob_livo_test {

int RunI2ContinuityTests(TestContext &context) {
  const double base = 500.0;
  prob_livo::ProbImuAdapter adapter(AdapterOptions());
  StatesGroup host;
  InitializeHostState(host);
  prob_livo::ProbESKF19 filter(host, FilterOptions());
  const auto init_samples = MakeImuSequence(
      base, 50, Eigen::Vector3d(0.0, 0.0, 9.7), Eigen::Vector3d::Zero());
  LidarMeasureGroup init_packet =
      MakeEpoch(base, base + 0.49, base, init_samples, {});
  const auto init = adapter.ProcessLioEpoch(
      init_packet, filter, prob_livo::SchedulerMode::kOnlyLio);
  context.Check(init.success && init.initialized,
                "continuity fixture failed initialization");

  double start = base + 0.49;
  std::vector<prob_livo::ImuSample> all_samples;
  for (int epoch = 0; epoch < 3; ++epoch) {
    const double end = start + 0.03;
    const std::vector<prob_livo::ImuSample> samples = {
        HostImu(start + 0.01, Eigen::Vector3d(0.2, 0.0, 9.7),
                Eigen::Vector3d(0.02, 0.01, 0.03)),
        HostImu(start + 0.02, Eigen::Vector3d(0.3, 0.1, 9.6),
                Eigen::Vector3d(0.03, 0.01, 0.04)),
        HostImu(end, Eigen::Vector3d(0.4, 0.2, 9.5),
                Eigen::Vector3d(0.04, 0.02, 0.05))};
    all_samples.insert(all_samples.end(), samples.begin(), samples.end());
    LidarMeasureGroup packet = MakeEpoch(start, end, start, samples, {});
    const auto result = adapter.ProcessLioEpoch(
        packet, filter, prob_livo::SchedulerMode::kOnlyLio);
    const std::string name = "i2.continuity.epoch" + std::to_string(epoch);
    context.Check(result.success && result.propagated,
                  name + " failed sequential propagation");
    context.Check(std::abs(result.epoch_start - start) < 1e-12 &&
                      std::abs(result.epoch_end - end) < 1e-12 &&
                      std::abs(packet.last_lio_update_time - end) < 1e-12,
                  name + " changed the epoch anchor incorrectly");
    if (epoch > 0) {
      context.Check(std::abs(adapter.propagation_trace().front().timestamp -
                             start) < 1e-12,
                    name + " did not start at the previous epoch endpoint");
    }
    start = end;
  }
  context.Check(std::abs(filter.current_time() - (base + 0.58)) < 1e-12,
                "three-epoch propagation did not reach the final endpoint");

  // Boundary IMU ownership is strict: the current scheduler measure may not
  // contain a sample after lio_time.
  LidarMeasureGroup bad_boundary = MakeEpoch(
      500.58, 500.60, 500.58,
      {HostImu(500.59, Eigen::Vector3d(0, 0, 9.7), Eigen::Vector3d::Zero()),
       HostImu(500.601, Eigen::Vector3d(0, 0, 9.7), Eigen::Vector3d::Zero())},
      {});
  context.Check(!adapter.ProcessLioEpoch(
                    bad_boundary, filter, prob_livo::SchedulerMode::kOnlyLio)
                     .success,
                "post-endpoint IMU boundary mutation was accepted");

  // Camera-cut seam: construct source points through the production rebase
  // helper, retain only its current bucket, and let the adapter consume the
  // resulting LIVO packet at the image capture endpoint.
  const double livo_start = 600.49;
  const double image_time = 600.53;
  PointType source_current;
  source_current.x = 1.0f;
  source_current.y = 2.0f;
  source_current.z = 3.0f;
  source_current.intensity = 11.0f;
  source_current.curvature = 15.0f;  // frame header 600.50 -> 600.515
  PointType source_next = source_current;
  source_next.x = 4.0f;
  source_next.intensity = 22.0f;
  source_next.curvature = 40.0f;  // frame header 600.50 -> 600.540
  PointType current_point;
  PointType next_point;
  context.Check(prob_livo::RebaseLivoPoint(
                    source_current, 600.50, livo_start, image_time,
                    current_point) == prob_livo::LivoPointBucket::kCurrent &&
                    prob_livo::RebaseLivoPoint(
                        source_next, 600.50, livo_start, image_time,
                        next_point) == prob_livo::LivoPointBucket::kNext,
                "camera-cut source points did not straddle image time");
  LidarMeasureGroup livo_packet = MakeEpoch(
      livo_start, image_time, livo_start,
      {HostImu(600.50, Eigen::Vector3d(0, 0, 9.7), Eigen::Vector3d::Zero()),
       HostImu(600.515, Eigen::Vector3d(0, 0, 9.7), Eigen::Vector3d::Zero()),
       HostImu(image_time, Eigen::Vector3d(0, 0, 9.7),
               Eigen::Vector3d::Zero())},
      {});
  livo_packet.pcl_proc_cur->points.push_back(current_point);
  livo_packet.pcl_proc_next->points.push_back(next_point);
  // Use a fresh adapter/filter so the seam test has the real init transition.
  prob_livo::ProbImuAdapter livo_adapter(AdapterOptions());
  StatesGroup livo_host;
  InitializeHostState(livo_host);
  prob_livo::ProbESKF19 livo_filter(livo_host, FilterOptions());
  LidarMeasureGroup livo_init = MakeEpoch(
      600.0, 600.49, 600.0,
      MakeImuSequence(600.0, 50, Eigen::Vector3d(0, 0, 9.7),
                      Eigen::Vector3d::Zero()),
      {});
  livo_adapter.ProcessLioEpoch(livo_init, livo_filter,
                               prob_livo::SchedulerMode::kLivo);
  const auto livo_result = livo_adapter.ProcessLioEpoch(
      livo_packet, livo_filter, prob_livo::SchedulerMode::kLivo);
  context.Check(livo_result.success && livo_result.propagated &&
                    livo_result.prob_scan_undistort_imu->size() == 1,
                "camera-cut seam did not consume current points only");
  context.Check(std::abs(livo_filter.current_time() - image_time) < 1e-12 &&
                    std::abs(livo_packet.last_lio_update_time - image_time) <
                        1e-12,
                "camera-cut seam did not reach image endpoint");
  context.Check(std::abs(livo_result.prob_scan_undistort_imu->points.front()
                             .intensity -
                         current_point.intensity) < 1e-6 &&
                    std::abs(livo_packet.pcl_proc_next->points.front().intensity -
                             next_point.intensity) < 1e-6,
                "camera-cut seam mixed current and next point identity");
  context.Check(std::abs(livo_start + current_point.curvature / 1000.0 -
                         600.515) < 1e-6 &&
                    std::abs(image_time + next_point.curvature / 1000.0 -
                             600.540) < 1e-6,
                "camera-cut rebase origins were not preserved");

  // Hybrid-authority guard: the staged adapter output has a unique name and
  // must not be wired to FAST's existing Process2/feats_undistort path.
  std::ifstream mapper_source(std::string(ROOT_DIR) + "src/LIVMapper.cpp");
  const std::string mapper_text((std::istreambuf_iterator<char>(mapper_source)),
                                std::istreambuf_iterator<char>());
  context.Check(mapper_text.find("p_imu->Process2") != std::string::npos &&
                    mapper_text.find("prob_scan_undistort_imu") ==
                        std::string::npos &&
                    mapper_text.find("ProcessLioEpoch") == std::string::npos,
                "Prob IMU-frame output was accidentally wired into FAST LIO");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
