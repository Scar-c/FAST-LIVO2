#ifndef PROB_LIVO_PROB_LIO_LIFECYCLE_H_
#define PROB_LIVO_PROB_LIO_LIFECYCLE_H_

#include "common_lib.h"

#include <cmath>
#include <cstddef>
#include <limits>

namespace prob_livo {

// Single production lifecycle authority.  LIVMapper is transport only;
// adapters and map/ESKF components consume this state.
enum class ProbLioLifecycle { IMU_INIT, MAP_INIT, RUN };

class ProbLioLifecycleAuthority {
 public:
  ProbLioLifecycle lifecycle() const { return lifecycle_; }
  std::size_t successful_epochs() const { return successful_epochs_; }
  double scheduler_epoch_anchor() const { return scheduler_epoch_anchor_; }

  // Seeding is not an epoch consumption and does not increment the counter.
  bool SeedSchedulerAnchor(LidarMeasureGroup &measures) {
    const double anchor = measures.last_lio_update_time;
    if (!std::isfinite(anchor) || anchor < 0.0) return false;
    if (std::isfinite(scheduler_epoch_anchor_) &&
        std::abs(anchor - scheduler_epoch_anchor_) > kTimeTolerance) {
      return false;
    }
    scheduler_epoch_anchor_ = anchor;
    return true;
  }

  // Exactly one call per successfully consumed epoch.  Failed work must not
  // call this function, so it cannot move the anchor or lifecycle state.
  bool CommitConsumedEpoch(LidarMeasureGroup &measures, double epoch_end) {
    if (!std::isfinite(epoch_end) || epoch_end < 0.0 ||
        !std::isfinite(scheduler_epoch_anchor_) ||
        epoch_end + kTimeTolerance < scheduler_epoch_anchor_ ||
        std::abs(measures.last_lio_update_time -
                 scheduler_epoch_anchor_) > kTimeTolerance) {
      return false;
    }
    measures.last_lio_update_time = epoch_end;
    scheduler_epoch_anchor_ = epoch_end;
    ++successful_epochs_;
    return true;
  }

  bool MarkFilterInitialized() {
    if (lifecycle_ != ProbLioLifecycle::IMU_INIT) return false;
    lifecycle_ = ProbLioLifecycle::MAP_INIT;
    return true;
  }

  bool MarkMapInitialized() {
    if (lifecycle_ != ProbLioLifecycle::MAP_INIT) return false;
    lifecycle_ = ProbLioLifecycle::RUN;
    return true;
  }

 private:
  static constexpr double kTimeTolerance = 2e-8;
  ProbLioLifecycle lifecycle_ = ProbLioLifecycle::IMU_INIT;
  std::size_t successful_epochs_ = 0;
  double scheduler_epoch_anchor_ = -std::numeric_limits<double>::infinity();
};

struct SchedulerImuSelection {
  deque<sensor_msgs::Imu::ConstPtr> current;
  sensor_msgs::Imu::ConstPtr lookahead;
};

// Consume exactly (epoch_start, epoch_end].  The first sample > endpoint is
// returned as lookahead and deliberately remains in imu_buffer.
inline bool ConsumeSchedulerImuEpoch(
    deque<sensor_msgs::Imu::ConstPtr> &imu_buffer, double epoch_start,
    double epoch_end, SchedulerImuSelection &selection,
    double tolerance = 2e-8, bool include_before_start = false) {
  selection.current.clear();
  selection.lookahead.reset();
  if (!std::isfinite(epoch_start) || !std::isfinite(epoch_end) ||
      epoch_end + tolerance < epoch_start) {
    return false;
  }
  while (!imu_buffer.empty()) {
    const auto &message = imu_buffer.front();
    if (message == nullptr) return false;
    const double timestamp = message->header.stamp.toSec();
    if (!std::isfinite(timestamp)) return false;
    if (timestamp <= epoch_start + tolerance) {
      if (include_before_start) {
        selection.current.push_back(message);
      }
      imu_buffer.pop_front();
    } else if (timestamp <= epoch_end + tolerance) {
      selection.current.push_back(message);
      imu_buffer.pop_front();
    } else {
      selection.lookahead = message;
      break;
    }
  }
  return true;
}

}  // namespace prob_livo

#endif  // PROB_LIVO_PROB_LIO_LIFECYCLE_H_
