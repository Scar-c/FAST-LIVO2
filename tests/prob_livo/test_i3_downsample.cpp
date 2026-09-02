#include "test_i3_support.h"

namespace prob_livo_test {

int RunI3DownsampleTests(TestContext &context) {
  PointCloudXYZI::Ptr input(new PointCloudXYZI());
  input->push_back(MakePoint(0.60f, 0.0f, 0.0f));
  input->push_back(MakePoint(0.51f, 0.0f, 0.0f));
  input->push_back(MakePoint(-0.24f, 0.0f, 0.0f));
  input->push_back(MakePoint(-0.14f, 0.0f, 0.0f));
  PointCloudXYZI::Ptr output(new PointCloudXYZI());
  LI2Sup::VoxelGridClosest<PointType> filter;
  filter.setLeafSize(0.5f);
  filter.setInputCloud(input);
  filter.filter(output);
  context.Check(output->size() == 2,
                "VoxelGridClosest did not produce one representative per voxel");
  bool found_closest = false;
  for (const auto &point : output->points) {
    if (std::abs(point.x - 0.51f) < 1e-6f) found_closest = true;
  }
  context.Check(found_closest,
                "VoxelGridClosest did not retain the closest point to voxel center");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
