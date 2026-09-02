#include "prob_livo/prob_lio_lifecycle.h"
#include "test_i2_support.h"

namespace prob_livo_test {

int RunI2SchedulerImuBufferTests(TestContext &context) {
  deque<sensor_msgs::Imu::ConstPtr> buffer;
  for (const auto &sample : MakeImuSequence(
           20.0, 7, Eigen::Vector3d(0.0, 0.0, 9.7),
           Eigen::Vector3d::Zero(), 0.01)) {
    buffer.push_back(ToRosImu(sample));
  }

  prob_livo::SchedulerImuSelection first;
  context.Check(prob_livo::ConsumeSchedulerImuEpoch(buffer, 20.0, 20.025,
                                                     first),
                "scheduler rejected first IMU epoch");
  context.Check(first.current.size() == 2 && first.lookahead &&
                    std::abs(first.lookahead->header.stamp.toSec() - 20.03) <
                        1e-12 &&
                    buffer.size() == 4,
                "scheduler did not preserve first lookahead");

  prob_livo::SchedulerImuSelection second;
  context.Check(prob_livo::ConsumeSchedulerImuEpoch(buffer, 20.025, 20.045,
                                                     second),
                "scheduler rejected second IMU epoch");
  context.Check(second.current.size() == 2 &&
                    second.current.front() == first.lookahead &&
                    second.lookahead &&
                    std::abs(second.lookahead->header.stamp.toSec() - 20.05) <
                        1e-12 &&
                    buffer.size() == 2,
                "scheduler duplicated, dropped, or popped lookahead");

  const std::size_t before = buffer.size();
  prob_livo::SchedulerImuSelection rejected;
  context.Check(!prob_livo::ConsumeSchedulerImuEpoch(buffer, 20.06, 20.05,
                                                      rejected) &&
                    buffer.size() == before,
                "invalid epoch mutated scheduler IMU buffer");
  return context.Passed() ? 0 : 1;
}

}  // namespace prob_livo_test
