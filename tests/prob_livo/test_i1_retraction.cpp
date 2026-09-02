#include "test_support.h"

namespace prob_livo_test {

namespace {

void CompareRetractionState(TestContext &context,
                            const OracleState &oracle,
                            const StatesGroup &host,
                            const std::string &prefix,
                            double expected_exposure) {
  CheckState(context, oracle, host, prefix, 2e-12);
  const double exposure_error = std::abs(host.inv_expo_time - expected_exposure);
  context.Record(prefix + ".exposure", exposure_error);
}

}  // namespace

int RunRetractionTests(TestContext &context) {
  StatesGroup host;
  InitializeHostState(host);
  const double gravity_norm = 9.81;
  OracleState oracle = ToOracle(host);

  Vector19 increment = Vector19::Zero();
  increment.segment<3>(Layout::kRot0) << 0.12, -0.07, 0.045;
  increment.segment<3>(Layout::kPos0) << 0.3, -0.1, 0.04;
  increment(Layout::kExpo) = 0.18;
  increment.segment<3>(Layout::kVel0) << -0.06, 0.11, 0.08;
  increment.segment<3>(Layout::kBg0) << 0.004, -0.003, 0.002;
  increment.segment<3>(Layout::kBa0) << -0.02, 0.01, 0.03;
  increment.segment<3>(Layout::kGrav0) << 0.25, -0.13, 0.08;

  const Vector18 physical_increment = prob_livo::ExtractPhysicalVector(increment);
  prob_livo_test_oracle::Apply(oracle, physical_increment, gravity_norm);
  prob_livo::ApplySuperLioIncrement19(host, increment, gravity_norm);
  CompareRetractionState(context, oracle, host, "retraction.dense", 1.88);
  context.Check(std::abs(host.gravity.norm() - gravity_norm) < 2e-12,
                "Super retraction did not normalize gravity");
  context.Check(std::abs(host.inv_expo_time - (1.7 + 0.18)) < 2e-12,
                "retraction did not apply exposure increment");

  StatesGroup small_host;
  InitializeHostState(small_host);
  OracleState small_oracle = ToOracle(small_host);
  Vector19 small_increment = Vector19::Zero();
  small_increment.segment<3>(Layout::kRot0) << 1e-8, -2e-8, 3e-8;
  small_increment.segment<3>(Layout::kPos0) << 1e-9, 2e-9, -1e-9;
  small_increment.segment<3>(Layout::kGrav0) << 1e-7, -2e-7, 1e-7;
  prob_livo_test_oracle::Apply(
      small_oracle, prob_livo::ExtractPhysicalVector(small_increment), gravity_norm);
  prob_livo::ApplySuperLioIncrement19(small_host, small_increment, gravity_norm);
  CompareRetractionState(context, small_oracle, small_host, "retraction.small", 1.7);

  // Negative fixture: FAST's visual ABI operator is intentionally additive in
  // gravity, while Super's LIO retraction projects it to the fixed norm.
  StatesGroup fast_operator_state;
  InitializeHostState(fast_operator_state);
  fast_operator_state += increment;
  const double fast_gravity_error =
      std::abs(fast_operator_state.gravity.norm() - gravity_norm);
  context.Record("retraction.negative_fast_operator", fast_gravity_error);
  context.Check(fast_gravity_error > 1e-3,
                "FAST operator+= unexpectedly matched Super gravity semantics");

  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
