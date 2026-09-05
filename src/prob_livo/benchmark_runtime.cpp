#include "prob_livo/benchmark_runtime.h"

#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>

namespace prob_livo {
namespace {

using Clock = std::chrono::steady_clock;

std::map<std::string, std::uint64_t> ReadKilobytes(
    const std::string &path) {
  std::ifstream input(path);
  std::map<std::string, std::uint64_t> values;
  std::string line;
  while (std::getline(input, line)) {
    const std::size_t colon = line.find(':');
    if (colon == std::string::npos) continue;
    std::istringstream parser(line.substr(colon + 1));
    std::uint64_t value = 0;
    if (parser >> value) values[line.substr(0, colon)] = value;
  }
  return values;
}

std::uint64_t Value(const std::map<std::string, std::uint64_t> &values,
                    const std::string &key) {
  const auto found = values.find(key);
  return found == values.end() ? 0 : found->second;
}

}  // namespace

double ProcessCpuSeconds() {
  timespec value{};
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &value) != 0) return 0.0;
  return static_cast<double>(value.tv_sec) +
         static_cast<double>(value.tv_nsec) * 1e-9;
}

BenchmarkProcessMonitor::~BenchmarkProcessMonitor() { Stop(); }

bool BenchmarkProcessMonitor::Start(const std::string &csv_path,
                                    const std::string &label,
                                    double interval_seconds) {
  if (thread_.joinable() || csv_path.empty() || !(interval_seconds > 0.0))
    return false;
  csv_path_ = csv_path;
  label_ = label;
  interval_seconds_ = interval_seconds;
  stop_.store(false);
  thread_ = std::thread(&BenchmarkProcessMonitor::Run, this);
  return true;
}

void BenchmarkProcessMonitor::SetSensorTimestamp(double timestamp) {
  if (std::isfinite(timestamp)) sensor_timestamp_.store(timestamp);
}

void BenchmarkProcessMonitor::Stop() {
  if (!thread_.joinable()) return;
  stop_.store(true);
  wake_.notify_all();
  thread_.join();
}

void BenchmarkProcessMonitor::Run() {
  std::ofstream output(csv_path_, std::ios::out | std::ios::trunc);
  if (!output) return;
  output << "elapsed_s,label,pid,sensor_timestamp,rss_kb,pss_kb,uss_kb,"
            "raw_cpu_pct\n";
  const auto start = Clock::now();
  auto previous_wall = start;
  double previous_cpu = ProcessCpuSeconds();
  bool final_sample = false;
  while (true) {
    const auto now = Clock::now();
    const double cpu = ProcessCpuSeconds();
    const double wall_delta =
        std::chrono::duration<double>(now - previous_wall).count();
    const double raw_cpu = wall_delta > 0.0
                               ? (cpu - previous_cpu) / wall_delta * 100.0
                               : 0.0;
    const auto status = ReadKilobytes("/proc/self/status");
    const auto smaps = ReadKilobytes("/proc/self/smaps_rollup");
    const std::uint64_t uss = Value(smaps, "Private_Clean") +
                              Value(smaps, "Private_Dirty");
    output << std::fixed << std::setprecision(6)
           << std::chrono::duration<double>(now - start).count() << ','
           << label_ << ',' << static_cast<long>(getpid()) << ','
           << std::setprecision(9) << sensor_timestamp_.load() << ','
           << Value(status, "VmRSS") << ',' << Value(smaps, "Pss") << ','
           << uss << ',' << std::setprecision(3) << raw_cpu << '\n';
    output.flush();
    previous_wall = now;
    previous_cpu = cpu;
    if (stop_.load()) {
      if (final_sample) break;
      final_sample = true;
      continue;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    wake_.wait_for(lock, std::chrono::duration<double>(interval_seconds_),
                   [this] { return stop_.load(); });
  }
}

}  // namespace prob_livo
