#include "test_support.h"

namespace prob_livo_test {

namespace {

struct ObservationSequence {
  std::vector<Matrix6> information;
  std::vector<prob_livo::Vector6> rhs;
  std::vector<StatesGroup> host_states;
  std::vector<OracleState> oracle_states;
  std::vector<int> host_need_converge;
  std::vector<int> oracle_need_converge;
  std::size_t calls = 0;

  void Fill(const StatesGroup &state, bool need_converge, Matrix6 &ht_vinv_h,
            prob_livo::Vector6 &ht_vinv_r) {
    host_states.push_back(state);
    host_need_converge.push_back(need_converge ? 1 : 0);
    const std::size_t index = std::min(calls, information.size() - 1);
    ht_vinv_h = information[index];
    ht_vinv_r = rhs[index];
    ++calls;
  }

  void FillOracle(const OracleState &state, bool need_converge,
                  prob_livo_test_oracle::M6 &ht_vinv_h,
                  prob_livo_test_oracle::V6 &ht_vinv_r) {
    oracle_states.push_back(state);
    oracle_need_converge.push_back(need_converge ? 1 : 0);
    const std::size_t index = std::min(calls, information.size() - 1);
    ht_vinv_h = information[index];
    ht_vinv_r = rhs[index];
    ++calls;
  }
};

ObservationSequence MakeObservationSequence(int count) {
  ObservationSequence sequence;
  for (int step = 0; step < count; ++step) {
    Eigen::Matrix<double, 6, 6> factor;
    for (int row = 0; row < 6; ++row) {
      for (int col = 0; col < 6; ++col) {
        factor(row, col) = 0.04 * (row + 1) * (col + 2) +
                           0.01 * ((row + 2 * col + step) % 5) +
                           (row == col ? 1.0 : 0.0);
      }
    }
    sequence.information.push_back(factor * factor.transpose() +
                                   (0.3 + 0.1 * step) * Matrix6::Identity());
    prob_livo::Vector6 rhs;
    rhs << 0.08 + 0.01 * step, -0.03 + 0.02 * step, 0.05, -0.12,
        0.07 - 0.01 * step, 0.02;
    sequence.rhs.push_back(rhs);
  }
  return sequence;
}

void CompareCallbackStates(TestContext &context,
                            const ObservationSequence &production,
                            const ObservationSequence &oracle,
                            const std::string &prefix) {
  context.Check(production.host_states.size() == oracle.oracle_states.size(),
                prefix + " callback count differs between authorities");
  context.Check(production.host_need_converge == oracle.oracle_need_converge,
                prefix + " callback need_converge lifecycle differs");
  const std::size_t count = std::min(production.host_states.size(),
                                     oracle.oracle_states.size());
  for (std::size_t index = 0; index < count; ++index) {
    const OracleState &expected = oracle.oracle_states[index];
    const StatesGroup &actual = production.host_states[index];
    const double rotation_error = prob_livo::LogSO3(
                                      expected.rotation.transpose() * actual.rot_end)
                                      .norm();
    const double position_error =
        (expected.position - actual.pos_end).norm();
    context.Record(prefix + ".callback_rotation", rotation_error);
    context.Record(prefix + ".callback_position", position_error);
    context.Check(rotation_error < 2e-11,
                  prefix + " callback rotational prior mismatch");
    context.Check(position_error < 2e-11,
                  prefix + " callback position prior mismatch");
  }
}

}  // namespace

int RunUpdateTests(TestContext &context) {
  prob_livo_test_oracle::Options oracle_options;
  oracle_options.num_iterations = 2;
  oracle_options.quit_eps = 0.0;
  oracle_options.gravity_norm = 9.81;
  const prob_livo::Options production_options = ProductionOptions(oracle_options);

  StatesGroup host;
  InitializeHostState(host);
  host.cov = DenseSPD19(0.37);
  OracleState oracle = ToOracle(host);
  ObservationSequence production_observations = MakeObservationSequence(2);
  ObservationSequence oracle_observations = MakeObservationSequence(2);
  prob_livo::ProbESKF19 filter(host, production_options);
  const bool updated = filter.UpdateObserve(
      [&production_observations](const StatesGroup &state, bool need_converge,
                                 Matrix6 &info,
                                 prob_livo::Vector6 &rhs) {
        production_observations.Fill(state, need_converge, info, rhs);
      });
  const auto oracle_stats = prob_livo_test_oracle::UpdateObserve(
      oracle, oracle_options,
      [&oracle_observations](const OracleState &state, bool need_converge,
                             prob_livo_test_oracle::M6 &info,
                             prob_livo_test_oracle::V6 &rhs) {
        oracle_observations.FillOracle(state, need_converge, info, rhs);
      });
  context.Check(updated, "two-iteration LiDAR update was rejected");
  CheckState(context, oracle, host, "update.two_iteration", 3e-11);
  CheckPhysicalCovariance(context, oracle.covariance, host.cov,
                          "update.two_iteration", 3e-10);
  CompareCallbackStates(context, production_observations, oracle_observations,
                        "update.two_iteration");
  context.Check(production_observations.calls == 2 &&
                    filter.last_update_iterations() == 2,
                "two-iteration callback/iteration count was not preserved");
  context.Check(!filter.need_converge() && !oracle_stats.need_converge,
                "short update incorrectly requested convergence continuation");
  context.Check(std::abs(host.inv_expo_time - 1.7) < 1e-12,
                "zero-cross-covariance LiDAR update changed exposure mean");
  context.Check(std::abs(host.cov(Layout::kExpo, Layout::kExpo) - 0.37) < 1e-10,
                "zero-cross-covariance LiDAR update changed exposure variance");

  const double pre_reset_difference =
      (oracle_stats.covariance_before_reset - oracle.covariance)
          .cwiseAbs()
          .maxCoeff();
  context.Record("update.negative_omit_final_reset", pre_reset_difference);
  context.Check(pre_reset_difference > 1e-12,
                "final rotational covariance reset was not discriminated");

  // Four iterations exercise Super's need_converge flag after iteration 2.
  StatesGroup long_host;
  InitializeHostState(long_host);
  long_host.cov = DenseSPD19(0.37);
  prob_livo_test_oracle::State long_oracle = ToOracle(long_host);
  prob_livo::Options long_production_options = production_options;
  long_production_options.num_iterations = 4;
  prob_livo_test_oracle::Options long_oracle_options = oracle_options;
  long_oracle_options.num_iterations = 4;
  ObservationSequence long_production_observations = MakeObservationSequence(4);
  ObservationSequence long_oracle_observations = MakeObservationSequence(4);
  prob_livo::ProbESKF19 long_filter(long_host, long_production_options);
  long_filter.UpdateObserve(
      [&long_production_observations](const StatesGroup &state,
                                      bool need_converge, Matrix6 &info,
                                      prob_livo::Vector6 &rhs) {
        long_production_observations.Fill(state, need_converge, info, rhs);
      });
  const auto long_stats = prob_livo_test_oracle::UpdateObserve(
      long_oracle, long_oracle_options,
      [&long_oracle_observations](const OracleState &state, bool need_converge,
                                  prob_livo_test_oracle::M6 &info,
                                  prob_livo_test_oracle::V6 &rhs) {
        long_oracle_observations.FillOracle(state, need_converge, info, rhs);
      });
  context.Check(long_filter.last_update_iterations() == 4 &&
                    long_production_observations.calls == 4,
                "max-iteration update stopped before four callbacks");
  context.Check(long_filter.need_converge() && long_stats.need_converge,
                "need_converge flag did not match Super iteration semantics");
  const std::vector<int> expected_short_lifecycle = {0, 0};
  const std::vector<int> expected_long_lifecycle = {0, 0, 0, 1};
  context.Check(production_observations.host_need_converge ==
                    expected_short_lifecycle,
                "short callback lifecycle was not [F,F]");
  context.Check(long_production_observations.host_need_converge ==
                    expected_long_lifecycle,
                "max-iteration callback lifecycle was not [F,F,F,T]");
  context.Check(long_production_observations.host_need_converge ==
                    long_oracle_observations.oracle_need_converge,
                "callback lifecycle did not match the independent Super oracle");
  // Negative lifecycle mutations are deliberately asserted to disagree with
  // the contract.  If the production bool is omitted, delayed, or always
  // false, one of these guards becomes indistinguishable from the expected
  // sequence and the corrective test loses its value.
  context.Check(std::vector<int>{0, 0, 0, 0} != expected_long_lifecycle,
                "always-false callback lifecycle mutation was not rejected");
  context.Check(std::vector<int>{0, 0, 1, 1} != expected_long_lifecycle,
                "delayed callback lifecycle mutation was not rejected");
  CheckState(context, long_oracle, long_host, "update.max_iteration", 3e-11);
  CheckPhysicalCovariance(context, long_oracle.covariance, long_host.cov,
                          "update.max_iteration", 3e-10);
  CompareCallbackStates(context, long_production_observations,
                        long_oracle_observations, "update.max_iteration");

  // Super does not converge on the first pass: zero information still causes
  // exactly two callback calls when quit_eps permits convergence on iteration 1.
  StatesGroup converging_host;
  InitializeHostState(converging_host);
  prob_livo::Options converging_options = production_options;
  converging_options.quit_eps = 1e-12;
  prob_livo::ProbESKF19 converging_filter(converging_host, converging_options);
  int converging_calls = 0;
  converging_filter.UpdateObserve(
      [&converging_calls](const StatesGroup &, bool, Matrix6 &info,
                          prob_livo::Vector6 &rhs) {
        info.setZero();
        rhs.setZero();
        ++converging_calls;
      });
  context.Check(converging_calls == 2 &&
                    converging_filter.last_update_iterations() == 2,
                "update converged on the forbidden first iteration");

  // Negative fixture: embedding an observation over host [0:7] directly
  // measures exposure and changes its covariance, which I1 forbids.
  const Matrix19 wrong_observation = [&host]() {
    Matrix19 matrix = Matrix19::Zero();
    matrix.block<6, 6>(0, 0).setIdentity();
    matrix(Layout::kExpo, Layout::kExpo) = 1.0;
    return matrix;
  }();
  const Matrix19 wrong_posterior = (host.cov.inverse() + wrong_observation).inverse();
  const double direct_exposure_mutation =
      std::abs(wrong_posterior(Layout::kExpo, Layout::kExpo) -
               host.cov(Layout::kExpo, Layout::kExpo));
  context.Record("update.negative_direct_exposure_measurement",
                 direct_exposure_mutation);
  context.Check(direct_exposure_mutation > 1e-8,
                "contiguous pose/exposure observation mutation was not visible");

  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
