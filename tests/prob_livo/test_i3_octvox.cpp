#include "test_i3_support.h"

namespace prob_livo_test {

int RunI3OctVoxTests(TestContext &context) {
  using Map = LI2Sup::OctVoxMap<LI2Sup::V3, LI2Sup::scalar>;
  Map map(Map::Options{0.5f, 1000});
  LI2Sup::VV3 points;
  points = {LI2Sup::V3(0.1f, 0.1f, 2.0f), LI2Sup::V3(0.6f, 0.1f, 2.0f),
            LI2Sup::V3(0.1f, 0.6f, 2.0f), LI2Sup::V3(0.6f, 0.6f, 2.0f),
            LI2Sup::V3(0.3f, 0.3f, 2.0f)};
  std::vector<Eigen::Matrix3d> covariances(points.size(),
                                            0.01 * Eigen::Matrix3d::Identity());
  map.insert(points, covariances);
  Map::KNNHeapType top_k;
  map.getTopK(LI2Sup::V3(0.3f, 0.3f, 2.0f), top_k);
  context.Check(top_k.count >= 4,
                "OctVoxMap HKNN query returned fewer than four neighbors");
  context.Check(top_k.covs_[0].allFinite(),
                "OctVoxMap did not carry covariance with representative point");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
