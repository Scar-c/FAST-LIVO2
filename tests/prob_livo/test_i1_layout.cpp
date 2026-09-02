#include "test_support.h"

#include <sstream>

namespace prob_livo_test {

int RunLayoutTests(TestContext &context) {
  static_assert(DIM_STATE == Layout::kDim19, "FAST state ABI must remain 19D");
  static_assert(Layout::kExpo == 6, "FAST inverse exposure index moved");
  static_assert(Layout::kPhysicalIndex[6] == 7,
                "Super velocity must skip host exposure");

  Vector18 physical;
  for (int i = 0; i < 18; ++i) {
    physical(i) = 0.125 * (i + 1) - 0.02 * (i % 3);
  }
  const Vector19 embedded = prob_livo::EmbedPhysicalVector(physical);
  const Vector18 extracted = prob_livo::ExtractPhysicalVector(embedded);
  context.Record("layout.vector_roundtrip",
                 (physical - extracted).cwiseAbs().maxCoeff());
  context.Check((physical - extracted).cwiseAbs().maxCoeff() == 0.0,
                "physical vector roundtrip failed");
  context.Check(embedded(Layout::kExpo) == 0.0,
                "vector embedding overwrote exposure slot");

  const Matrix18 physical_cov = DenseSPD18();
  const Matrix19 host_cov = prob_livo::EmbedPhysicalCovariance(physical_cov);
  const Matrix18 recovered_cov =
      prob_livo::ExtractPhysicalCovariance(host_cov);
  const double covariance_error =
      (physical_cov - recovered_cov).cwiseAbs().maxCoeff();
  context.Record("layout.covariance_roundtrip", covariance_error);
  context.Check(covariance_error == 0.0, "physical covariance roundtrip failed");
  context.Check(host_cov(Layout::kExpo, Layout::kExpo) == 0.0,
                "physical covariance embedding did not reserve exposure");

  Matrix19 adversarial_host = host_cov;
  adversarial_host(Layout::kPhysicalIndex[6], Layout::kPhysicalIndex[10]) += 0.37;
  adversarial_host(Layout::kPhysicalIndex[10], Layout::kPhysicalIndex[6]) -= 0.19;
  const Matrix18 adversarial_recovered =
      prob_livo::ExtractPhysicalCovariance(adversarial_host);
  context.Check(adversarial_recovered(6, 10) != physical_cov(6, 10),
                "velocity/bias off-diagonal mutation was not observable");

  Matrix19 fx_host = prob_livo::EmbedPhysicalFx(physical_cov);
  const Matrix18 fx_recovered = prob_livo::ExtractPhysicalCovariance(fx_host);
  context.Check((fx_recovered - physical_cov).cwiseAbs().maxCoeff() == 0.0,
                "F_X embedding dropped a physical entry");
  context.Check(fx_host.row(Layout::kExpo).cwiseAbs().maxCoeff() == 1.0 &&
                    fx_host.col(Layout::kExpo).cwiseAbs().maxCoeff() == 1.0,
                "F_X exposure identity was not preserved");
  context.Check(fx_host(Layout::kExpo, Layout::kPhysicalIndex[0]) == 0.0 &&
                    fx_host(Layout::kPhysicalIndex[0], Layout::kExpo) == 0.0,
                "F_X introduced forbidden exposure transition");

  prob_livo::Matrix18x12 fw;
  for (int row = 0; row < 18; ++row) {
    for (int col = 0; col < 12; ++col) {
      fw(row, col) = 0.01 * (row + 1) - 0.003 * (col + 1);
    }
  }
  const prob_livo::HostFw fw_host = prob_livo::EmbedPhysicalFw(fw);
  for (int row = 0; row < 18; ++row) {
    context.Check((fw_host.row(Layout::kPhysicalIndex[static_cast<std::size_t>(row)]) -
                   fw.row(row))
                      .cwiseAbs()
                      .maxCoeff() == 0.0,
                  "F_W embedding changed process-noise placement");
  }
  context.Check(fw_host.row(Layout::kExpo).cwiseAbs().maxCoeff() == 0.0,
                "F_W directly drove exposure");

  // Negative fixture: a tempting contiguous host[0:18] interpretation shifts
  // v/bg/ba/g into exposure and is measurably different for this dense input.
  Vector18 wrong_contiguous = embedded.head<18>();
  const double wrong_mapping_error =
      (wrong_contiguous - physical).cwiseAbs().maxCoeff();
  context.Record("layout.negative_contiguous_mapping", wrong_mapping_error);
  context.Check(wrong_mapping_error > 1e-3,
                "negative contiguous mapping did not fail adversarial fixture");

  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
