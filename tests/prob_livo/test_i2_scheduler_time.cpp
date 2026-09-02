#include "test_i2_support.h"

#include <fstream>

namespace prob_livo_test {

int RunI2SchedulerTests(TestContext &context) {
  // ONLY_LIO keeps raw curvature relative to the current scan header, while
  // the state interval starts at the previous successful LIO endpoint.
  LidarMeasureGroup only = MakeEpoch(
      20.100, 20.200, 20.150,
      {HostImu(20.150, Eigen::Vector3d(0, 0, 9.7), Eigen::Vector3d::Zero()),
       HostImu(20.200, Eigen::Vector3d(0, 0, 9.7), Eigen::Vector3d::Zero())},
      {{20.160, 1}, {20.200, 2}});
  prob_livo::EpochTiming only_timing;
  std::string message;
  context.Check(prob_livo::ProbImuAdapter::ResolveEpochTiming(
                    only, prob_livo::SchedulerMode::kOnlyLio, only_timing,
                    message),
                "ONLY_LIO scheduler timing was rejected");
  context.Check(std::abs(only_timing.epoch_start - 20.100) < 1e-12 &&
                    std::abs(only_timing.epoch_end - 20.200) < 1e-12 &&
                    std::abs(only_timing.point_time_origin - 20.150) < 1e-12,
                "ONLY_LIO point/header timing contract changed");

  // This is the production cut helper used by sync_packages.  Feed it source
  // frame offsets on both sides of the camera capture boundary, then verify
  // the helper's current/next partition and both rebased origins.
  const double previous_lio = 30.000;
  const double frame_header = 30.040;
  const double camera_capture = 30.080;
  PointType source_before;
  source_before.curvature = 20.0f;  // source time = 30.060
  source_before.intensity = 101.0f;
  PointType source_after;
  source_after.curvature = 50.0f;  // source time = 30.090
  source_after.intensity = 202.0f;
  PointType current_point;
  PointType next_point;
  const auto before_bucket = prob_livo::RebaseLivoPoint(
      source_before, frame_header, previous_lio, camera_capture,
      current_point);
  const auto after_bucket = prob_livo::RebaseLivoPoint(
      source_after, frame_header, previous_lio, camera_capture, next_point);
  context.Check(before_bucket == prob_livo::LivoPointBucket::kCurrent &&
                    after_bucket == prob_livo::LivoPointBucket::kNext,
                "LIVO camera cut did not preserve current/next partition");
  context.Check(std::abs(previous_lio + current_point.curvature / 1000.0 -
                         30.060) < 1e-6,
                "LIVO current point was not rebased to previous LIO time");
  context.Check(std::abs(camera_capture + next_point.curvature / 1000.0 -
                         30.090) < 1e-6,
                "LIVO next point was not rebased to camera capture time");
  context.Check(std::abs(current_point.intensity - 101.0f) < 1e-6 &&
                    std::abs(next_point.intensity - 202.0f) < 1e-6,
                "scheduler point identity was changed during the cut");

  LidarMeasureGroup livo = MakeEpoch(
      previous_lio, camera_capture, previous_lio,
      {HostImu(30.010, Eigen::Vector3d(0, 0, 9.7), Eigen::Vector3d::Zero()),
       HostImu(30.080, Eigen::Vector3d(0, 0, 9.7), Eigen::Vector3d::Zero())},
      {{30.060, 101}});
  prob_livo::EpochTiming livo_timing;
  context.Check(prob_livo::ProbImuAdapter::ResolveEpochTiming(
                    livo, prob_livo::SchedulerMode::kLivo, livo_timing,
                    message),
                "LIVO scheduler timing was rejected");
  context.Check(std::abs(livo_timing.epoch_start - previous_lio) < 1e-12 &&
                    std::abs(livo_timing.epoch_end - camera_capture) < 1e-12 &&
                    std::abs(livo_timing.point_time_origin - previous_lio) <
                        1e-12,
                "LIVO rebased point origin was not the previous LIO time");

  // Negative mutations: source-header timing, camera-as-both-origin, and
  // post-cut insertion all disagree with the production contract.
  context.Check(std::abs(frame_header + current_point.curvature / 1000.0 -
                         30.060) > 1e-3,
                "original frame-header origin mutation was not discriminated");
  context.Check(std::abs(camera_capture + current_point.curvature / 1000.0 -
                         30.060) > 1e-3,
                "camera-as-current-origin mutation was not discriminated");
  context.Check(after_bucket != prob_livo::LivoPointBucket::kCurrent,
                "post-cut point mutation was not discriminated");

  // Source-level guard: production sync_packages must call the helper that
  // owns this formula, rather than silently regrowing the old inline branch.
  std::ifstream scheduler_source(std::string(ROOT_DIR) + "src/LIVMapper.cpp");
  const std::string scheduler_text(
      (std::istreambuf_iterator<char>(scheduler_source)),
      std::istreambuf_iterator<char>());
  context.Check(scheduler_text.find("prob_livo::RebaseLivoPoint") !=
                    std::string::npos,
                "production sync_packages does not use the scheduler helper");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
