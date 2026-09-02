#include "test_i3_support.h"

namespace prob_livo_test {

int RunI3HknnTests(TestContext &context) {
  context.Check(flat_search_order_offsets.size() == 7 &&
                    flat_search_order_offsets.front() == 0 &&
                    flat_search_order_offsets.back() == 593,
                "HKNN 60-neighbor search table is not the production table");
  context.Check(HKNN_neighbor_voxel.size() == 60 &&
                    orders_min_dis2[0] == 0.0625f,
                "HKNN neighbor/radius constants changed");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
