#include <cmath>
#include <cstddef>
#include <iostream>

float resolve_cached_mean(const float *patch, std::size_t count, float &cached,
                          int &compute_calls)
{
  float mean = cached;
  if (std::abs(mean) < 1e-6f) {
    float sum = 0.0f;
    for (std::size_t i = 0; i < count; ++i) sum += patch[i];
    mean = sum / static_cast<float>(count);
    cached = mean;
    ++compute_calls;
  }
  return mean;
}

int main()
{
  const float patch[] = {1.0f, 2.0f, 3.0f, 4.0f};
  float cached = 0.0f;
  int compute_calls = 0;
  const float first = resolve_cached_mean(patch, 4, cached, compute_calls);
  const float second = resolve_cached_mean(patch, 4, cached, compute_calls);
  if (std::abs(first - 2.5f) > 1e-6f ||
      std::abs(second - 2.5f) > 1e-6f || compute_calls != 1) {
    std::cerr << "cached-mean oracle failed\n";
    return 1;
  }
  return 0;
}

