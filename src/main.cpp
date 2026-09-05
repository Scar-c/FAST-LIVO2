#include "LIVMapper.h"

#include <cstdlib>
#include <string>

#include <omp.h>
#include <tbb/global_control.h>

namespace {

int WorkerLimit()
{
  const char *raw = std::getenv("PROB_LIVO_WORKERS");
  if (!raw || !*raw) return 4;
  try {
    const int value = std::stoi(raw);
    return value > 0 ? value : 4;
  } catch (...) {
    return 4;
  }
}

}  // namespace

int main(int argc, char **argv)
{
  const int worker_limit = WorkerLimit();
  tbb::global_control tbb_control(
      tbb::global_control::max_allowed_parallelism, worker_limit);
  omp_set_num_threads(worker_limit);
  ros::init(argc, argv, "laserMapping");
  ros::NodeHandle nh;
  image_transport::ImageTransport it(nh);
  LIVMapper mapper(nh); 
  mapper.initializeSubscribersAndPublishers(nh, it);
  ROS_INFO("runtime worker limit: TBB/OMP=%d", worker_limit);
  mapper.run();
  return 0;
}
