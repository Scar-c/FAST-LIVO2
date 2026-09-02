#include "test_i2_support.h"

namespace prob_livo_test {
int RunI2SchedulerTests(TestContext &context);
int RunI2InitializationTests(TestContext &context);
int RunI2PropagationTests(TestContext &context);
int RunI2UndistortionTests(TestContext &context);
int RunI2ContinuityTests(TestContext &context);
int RunI2LifecycleHandoffTests(TestContext &context);
int RunI2SchedulerImuBufferTests(TestContext &context);
}  // namespace prob_livo_test

int main() {
  prob_livo_test::TestContext scheduler;
  prob_livo_test::RunI2SchedulerTests(scheduler);
  scheduler.Print("G-I2.1 scheduler time contract");

  prob_livo_test::TestContext imu;
  prob_livo_test::RunI2InitializationTests(imu);
  prob_livo_test::RunI2PropagationTests(imu);
  imu.Print("G-I2.2/G-I2.3/G-I2.4 endpoint, init, propagation");

  prob_livo_test::TestContext undistort;
  prob_livo_test::RunI2UndistortionTests(undistort);
  undistort.Print("G-I2.5/G-I2.6 undistortion and identity");

  prob_livo_test::TestContext continuity;
  prob_livo_test::RunI2ContinuityTests(continuity);
  prob_livo_test::RunI2LifecycleHandoffTests(continuity);
  prob_livo_test::RunI2SchedulerImuBufferTests(continuity);
  continuity.Print("G-I2.7/G-I2.8/G-I2.9 continuity, camera cut, authority");

  return scheduler.Passed() && imu.Passed() && undistort.Passed() &&
                 continuity.Passed()
             ? 0
             : 1;
}
