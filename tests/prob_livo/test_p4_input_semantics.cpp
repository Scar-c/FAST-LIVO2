#include "preprocess.h"
#include "prob_livo/input_semantics.h"
#include "test_i3_support.h"

#include <cmath>
#include <limits>

namespace prob_livo_test {

namespace {

sensor_msgs::PointCloud2::ConstPtr MakeOusterFixture() {
  pcl::PointCloud<ouster_ros::Point> source;
  source.points.resize(25);
  source.width = source.points.size();
  source.height = 1;
  source.is_dense = false;
  for (std::size_t index = 0; index < source.points.size(); ++index) {
    auto &point = source.points[index];
    point.x = static_cast<float>(index + 10);
    point.y = 0.0f;
    point.z = 0.0f;
    point.intensity = static_cast<float>(100.0 + index);
    point.t = static_cast<std::uint32_t>(index + 1) * 1000000U;
    point.ring = static_cast<std::uint8_t>(index % 16);
  }
  source.points[0].x = 1.0f;       // below blind
  source.points[3].x = 3.0f;       // accepted
  source.points[9].x = 149.0f;     // accepted, nonmonotonic time
  source.points[12].x = 150.0f;   // strict maxrange rejection
  source.points[15].x = 151.0f;   // maxrange rejection
  source.points[18].x = 2.0001f;  // just above blind
  source.points[21].x = 2.0f;    // strict blind rejection
  source.points[24].x = 2.0001f;
  source.points[3].t = 9000000U;
  source.points[9].t = 1000000U;
  source.points[18].t = 7000000U;
  source.points[24].t = 2000000U;
  source.points[6].x = std::numeric_limits<float>::quiet_NaN();

  sensor_msgs::PointCloud2::Ptr message(new sensor_msgs::PointCloud2());
  pcl::toROSMsg(source, *message);
  message->header.stamp = ros::Time(123.456789);
  return message;
}

}  // namespace

int RunP4InputSemanticsTests(TestContext &context) {
  const auto message = MakeOusterFixture();
  Preprocess preprocess;
  PointCloudXYZI::Ptr output(new PointCloudXYZI());
  preprocess.process_super_ntu_legacy(message, output, 2.0, 150.0, 3);

  const std::vector<int> expected_source_indices{3, 9, 18, 24};
  context.Check(output->size() == expected_source_indices.size(),
                "legacy fixture accepted-point count mismatch");
  for (std::size_t index = 0;
       index < std::min(output->size(), expected_source_indices.size());
       ++index) {
    const int source_index = expected_source_indices[index];
    const auto &point = output->points[index];
    context.Check(std::abs(point.intensity - (100.0f + source_index)) < 1e-6f,
                  "legacy source-index stride/order mismatch");
    context.Check(std::abs(point.x -
                           (source_index == 3 ? 3.0f :
                            source_index == 9 ? 149.0f : 2.0001f)) < 1e-4f,
                  "legacy XYZ selection mismatch");
  }
  const std::vector<float> expected_times_ms{9.0f, 1.0f, 7.0f, 2.0f};
  for (std::size_t index = 0;
       index < std::min(output->size(), expected_times_ms.size()); ++index) {
    context.Check(std::abs(output->points[index].curvature -
                           expected_times_ms[index]) < 1e-6f,
                  "legacy point-time conversion/order mismatch");
  }

  // These assertions are deliberately discriminating: each one fails for a
  // 1 m blind cut, an omitted 150 m cut, a native ring/sort path, or an
  // off-by-one stride.
  context.Check(output->points.front().intensity == 103.0f,
                "legacy first accepted point is not source index 3");
  context.Check(output->points.back().intensity == 124.0f,
                "legacy last accepted point is not source index 24");
  context.Check(std::abs(output->points.front().curvature - 9.0f) < 1e-6f &&
                    output->points[1].curvature == 1.0f,
                "legacy source order was replaced by time sorting");
  context.Check(std::isfinite(output->points[0].x),
                "legacy finite-point contract failed");

  prob_livo::InputSemantics semantics;
  context.Check(prob_livo::ParseInputSemantics("super_ntu_legacy", semantics) &&
                    semantics == prob_livo::InputSemantics::kSuperNtuLegacy,
                "Super input semantics mode did not parse");
  const double header = message->header.stamp.toSec();
  context.Check(std::abs(prob_livo::LidarHeaderTime(
                             semantics, header, 0.0) - header) < 1e-15,
                "legacy header timestamp was shifted");
  context.Check(std::abs(prob_livo::LidarHeaderTime(
                             semantics, header, -0.1) - header) > 0.05,
                "negative timestamp-offset mutation was not observable");
  const double end = prob_livo::LidarEndTime(*output, header);
  context.Check(std::abs(end - (header + 0.002)) < 1e-9,
                "legacy scan end did not use the last accepted source point");
  for (const auto &point : output->points) {
    context.Check(std::isfinite(header + point.curvature / 1000.0),
                  "legacy absolute point acquisition time is non-finite");
  }
  context.Record("G-P4.1 accepted_points", static_cast<double>(output->size()));
  context.Record("G-P4.2 legacy_scan_end", end);
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
