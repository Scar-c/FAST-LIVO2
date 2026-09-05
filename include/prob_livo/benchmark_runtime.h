#ifndef PROB_LIVO_BENCHMARK_RUNTIME_H_
#define PROB_LIVO_BENCHMARK_RUNTIME_H_

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

namespace prob_livo {

struct BenchmarkRuntimeCounters {
  std::uint64_t imu_callbacks_received = 0;
  std::uint64_t lidar_callbacks_received = 0;
  std::uint64_t image_callbacks_received = 0;
  std::uint64_t imu_messages_enqueued = 0;
  std::uint64_t lidar_messages_enqueued = 0;
  std::uint64_t image_messages_enqueued = 0;
  std::uint64_t ignored_input_messages = 0;
  std::uint64_t scheduler_step_calls = 0;
  std::uint64_t scheduler_poll_calls = 0;
  std::uint64_t scheduler_sync_packages = 0;
  std::uint64_t lidar_epochs = 0;
  std::uint64_t camera_epochs = 0;
};

struct BenchmarkRuntimeTiming {
  double input_preprocess_s = 0.0;
  double prob_lio_backend_s = 0.0;
  double visual_processing_s = 0.0;
  double estimator_compute_s = 0.0;
  double estimator_cpu_s = 0.0;
  std::uint64_t input_preprocess_count = 0;
  std::uint64_t prob_lio_backend_count = 0;
  std::uint64_t visual_processing_count = 0;
  std::uint64_t estimator_compute_count = 0;
};

double ProcessCpuSeconds();

class BenchmarkProcessMonitor {
 public:
  BenchmarkProcessMonitor() = default;
  ~BenchmarkProcessMonitor();
  bool Start(const std::string &csv_path, const std::string &label,
             double interval_seconds = 2.0);
  void SetSensorTimestamp(double timestamp);
  void Stop();

 private:
  void Run();

  std::string csv_path_;
  std::string label_;
  double interval_seconds_ = 2.0;
  std::atomic<double> sensor_timestamp_{0.0};
  std::atomic<bool> stop_{false};
  std::mutex mutex_;
  std::condition_variable wake_;
  std::thread thread_;
};

}  // namespace prob_livo

#endif  // PROB_LIVO_BENCHMARK_RUNTIME_H_
