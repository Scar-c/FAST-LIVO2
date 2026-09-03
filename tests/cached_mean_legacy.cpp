#include <cmath>
#include <cstddef>

float legacy_cached_mean(const float *patch, std::size_t count, float cached)
{
  float mean;
  if (std::abs(cached) < 1e-6f) {
    float sum = 0.0f;
    for (std::size_t i = 0; i < count; ++i) sum += patch[i];
    mean = sum / static_cast<float>(count);
  }
  return mean;
}

