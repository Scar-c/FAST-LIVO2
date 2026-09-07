#include "visual_memory.h"

#include "feature.h"
#include "visual_point.h"

#include <algorithm>
#include <cmath>
#include <limits>

std::atomic<std::size_t> VisualPatch::live_bytes_{0};
std::atomic<std::size_t> VisualPatch::live_patches_{0};
std::atomic<std::size_t> VisualPatch::allocations_{0};
std::atomic<std::size_t> VisualPatch::releases_{0};

VisualPatch::VisualPatch(std::size_t value_count)
    : values_(new float[value_count]()), value_count_(value_count)
{
  live_bytes_.fetch_add(bytes(), std::memory_order_relaxed);
  live_patches_.fetch_add(1, std::memory_order_relaxed);
  allocations_.fetch_add(1, std::memory_order_relaxed);
}

VisualPatch::~VisualPatch()
{
  live_bytes_.fetch_sub(bytes(), std::memory_order_relaxed);
  live_patches_.fetch_sub(1, std::memory_order_relaxed);
  releases_.fetch_add(1, std::memory_order_relaxed);
}

std::size_t VisualPatch::live_bytes()
{
  return live_bytes_.load(std::memory_order_relaxed);
}

std::size_t VisualPatch::live_patches()
{
  return live_patches_.load(std::memory_order_relaxed);
}

std::size_t VisualPatch::allocations()
{
  return allocations_.load(std::memory_order_relaxed);
}

std::size_t VisualPatch::releases()
{
  return releases_.load(std::memory_order_relaxed);
}

uint8_t VisualParentLocalIndex(const VOXEL_LOCATION &parent,
                               const V3D &position)
{
  uint8_t local_index = 0;
  for (int axis = 0; axis < 3; ++axis) {
    const int64_t fine = static_cast<int64_t>(
        std::floor(position[axis] / 0.25));
    const int64_t parent_axis = axis == 0 ? parent.x : (axis == 1 ? parent.y : parent.z);
    const int64_t local = fine - 2 * parent_axis;
    if (local == 1) local_index |= static_cast<uint8_t>(1u << axis);
  }
  return local_index;
}

VisualParentHost::~VisualParentHost()
{
  for (VisualPoint *point : voxel_points) delete point;
  voxel_points.clear();
  for (auto &attachments : local_attachments) attachments.clear();
}

void VisualParentHost::add(VisualPoint *point, uint8_t local_index)
{
  voxel_points.push_back(point);
  local_attachments[local_index & 7u].push_back(point);
  ++count;
}

VisualParentRegistry::VisualParentRegistry(std::size_t capacity)
    : capacity_(std::max<std::size_t>(1, capacity))
{
}

VisualParentRegistry::~VisualParentRegistry() { clear(); }

void VisualParentRegistry::touch(const VOXEL_LOCATION &key)
{
  const auto position = lru_positions_.find(key);
  if (position == lru_positions_.end()) return;
  lru_.splice(lru_.begin(), lru_, position->second);
  position->second = lru_.begin();
}

VisualParentHost *VisualParentRegistry::findNoTouch(
    const VOXEL_LOCATION &key) const
{
  const auto found = owners_.find(key);
  return found == owners_.end() ? nullptr : found->second.get();
}

VisualParentHost *VisualParentRegistry::getOrCreateForInsert(
    const VOXEL_LOCATION &key)
{
  const auto found = owners_.find(key);
  if (found != owners_.end()) {
    touch(key);
    return found->second.get();
  }

  auto owner = std::make_unique<VisualParentHost>();
  VisualParentHost *host = owner.get();
  owners_.emplace(key, std::move(owner));
  lru_.push_front(key);
  lru_positions_[key] = lru_.begin();
  return host;
}

void VisualParentRegistry::addPoint(const VOXEL_LOCATION &key,
                                     VisualPoint *point,
                                     uint8_t local_index)
{
  getOrCreateForInsert(key)->add(point, local_index);
}

std::size_t VisualParentRegistry::evictToCapacity(
    const std::function<void(const VOXEL_LOCATION &)> &before_destroy)
{
  std::size_t evicted = 0;
  while (owners_.size() > capacity_ && !lru_.empty()) {
    const VOXEL_LOCATION key = lru_.back();
    lru_.pop_back();
    lru_positions_.erase(key);
    if (before_destroy) before_destroy(key);
    owners_.erase(key);
    ++evicted;
  }
  return evicted;
}

void VisualParentRegistry::clear(
    const std::function<void(const VOXEL_LOCATION &)> &before_destroy)
{
  while (!lru_.empty()) {
    const VOXEL_LOCATION key = lru_.back();
    lru_.pop_back();
    lru_positions_.erase(key);
    if (before_destroy) before_destroy(key);
    owners_.erase(key);
  }
  owners_.clear();
}

void VisualParentRegistry::setCapacity(std::size_t capacity)
{
  capacity_ = std::max<std::size_t>(1, capacity);
}

std::vector<VOXEL_LOCATION> VisualParentRegistry::lruOrderForTest() const
{
  return std::vector<VOXEL_LOCATION>(lru_.begin(), lru_.end());
}

VisualMemorySnapshot VisualParentRegistry::snapshot() const
{
  VisualMemorySnapshot result;
  result.parent_hosts = owners_.size();
  for (const auto &entry : owners_) {
    const VisualParentHost &host = *entry.second;
    result.visual_points += host.voxel_points.size();
    result.vp_parent_max = std::max(result.vp_parent_max,
                                     host.voxel_points.size());
    std::size_t host_features = 0;
    std::size_t host_patch_bytes = 0;
    for (const VisualPoint *point : host.voxel_points) {
      if (point == nullptr) continue;
      result.observations += point->obs_.size();
      host_features += point->obs_.size();
      for (const Feature *feature : point->obs_) {
        if (feature == nullptr || feature->patch_owner_ == nullptr) continue;
        host_patch_bytes += feature->patch_owner_->bytes();
      }
    }
    result.features += host_features;
    result.patch_bytes += host_patch_bytes;
    result.feature_parent_max = std::max(result.feature_parent_max,
                                         host_features);
    result.patch_parent_max = std::max(result.patch_parent_max,
                                       host_patch_bytes);
  }
  return result;
}

VisualMemorySnapshot SnapshotVisualIndex(const VisualParentIndex &index)
{
  VisualMemorySnapshot result;
  result.parent_hosts = index.size();
  for (const auto &entry : index) {
    const VisualParentHost *host = entry.second;
    if (host == nullptr) continue;
    result.visual_points += host->voxel_points.size();
    result.vp_parent_max = std::max(result.vp_parent_max,
                                    host->voxel_points.size());
    std::size_t host_features = 0;
    std::size_t host_patch_bytes = 0;
    for (const VisualPoint *point : host->voxel_points) {
      if (point == nullptr) continue;
      result.observations += point->obs_.size();
      host_features += point->obs_.size();
      for (const Feature *feature : point->obs_) {
        if (feature == nullptr || feature->patch_owner_ == nullptr) continue;
        host_patch_bytes += feature->patch_owner_->bytes();
      }
    }
    result.features += host_features;
    result.patch_bytes += host_patch_bytes;
    result.feature_parent_max = std::max(result.feature_parent_max,
                                         host_features);
    result.patch_parent_max = std::max(result.patch_parent_max,
                                       host_patch_bytes);
  }
  return result;
}
