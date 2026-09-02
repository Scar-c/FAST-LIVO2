#include "prob_livo/prob_lio_lifecycle.h"
#include "test_i2_support.h"

namespace prob_livo_test {

int RunI2LifecycleHandoffTests(TestContext &context) {
  prob_livo::ProbLioLifecycleAuthority owner;
  LidarMeasureGroup packet = MakeEpoch(10.0, 10.1, 10.0, {}, {});
  context.Check(owner.SeedSchedulerAnchor(packet),
                "lifecycle owner rejected initial scheduler anchor");
  context.Check(owner.lifecycle() == prob_livo::ProbLioLifecycle::IMU_INIT,
                "lifecycle did not start in IMU_INIT");
  context.Check(owner.CommitConsumedEpoch(packet, 10.1),
                "IMU_INIT epoch was not committed");
  context.Check(owner.successful_epochs() == 1 &&
                    std::abs(owner.scheduler_epoch_anchor() - 10.1) < 1e-12,
                "IMU_INIT epoch advanced scheduler anchor incorrectly");
  context.Check(owner.MarkFilterInitialized() &&
                    owner.lifecycle() == prob_livo::ProbLioLifecycle::MAP_INIT,
                "filter initialization did not hand off to MAP_INIT");

  // A stale packet cannot commit and cannot mutate the current anchor.
  packet.last_lio_update_time = 10.0;
  const double before = owner.scheduler_epoch_anchor();
  context.Check(!owner.CommitConsumedEpoch(packet, 10.2) &&
                    std::abs(owner.scheduler_epoch_anchor() - before) < 1e-12 &&
                    owner.successful_epochs() == 1,
                "failed lifecycle handoff mutated scheduler state");

  packet.last_lio_update_time = 10.1;
  context.Check(owner.CommitConsumedEpoch(packet, 10.2),
                "MAP_INIT epoch was not committed");
  context.Check(owner.MarkMapInitialized() &&
                    owner.lifecycle() == prob_livo::ProbLioLifecycle::RUN,
                "map initialization did not hand off to RUN");
  packet.last_lio_update_time = 10.2;
  context.Check(owner.CommitConsumedEpoch(packet, 10.3) &&
                    owner.successful_epochs() == 3 &&
                    std::abs(packet.last_lio_update_time - 10.3) < 1e-12,
                "RUN anchor did not advance exactly once");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
