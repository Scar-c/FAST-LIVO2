#ifndef FAST_LIVO_NATIVE_BENCHMARK_MONITOR_H_
#define FAST_LIVO_NATIVE_BENCHMARK_MONITOR_H_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace fast_livo {

double ProcessCpuSeconds();

class NativeBenchmarkProcessMonitor {
 public:
  NativeBenchmarkProcessMonitor() = default;
  ~NativeBenchmarkProcessMonitor();
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

}  // namespace fast_livo

#endif
