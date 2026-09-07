#ifndef FAST_LIVO_VISUAL_MEMORY_H_
#define FAST_LIVO_VISUAL_MEMORY_H_

#include "voxel_map.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

class Feature;
class VisualPoint;

enum class VisualMemoryStage
{
  kCurrent,
  kLeakFix,
  kParentLru
};

// Owns one complete photometric patch.  The owner is deliberately separate
// from Feature so the temporary-patch path can transfer ownership explicitly.
// This also gives the memory tests a narrow, allocation-accountable seam.
class VisualPatch
{
public:
  explicit VisualPatch(std::size_t value_count);
  ~VisualPatch();

  VisualPatch(const VisualPatch &) = delete;
  VisualPatch &operator=(const VisualPatch &) = delete;

  float *data() { return values_.get(); }
  const float *data() const { return values_.get(); }
  std::size_t value_count() const { return value_count_; }
  std::size_t bytes() const { return value_count_ * sizeof(float); }

  static std::unique_ptr<VisualPatch> Allocate(std::size_t value_count)
  {
    return std::unique_ptr<VisualPatch>(new VisualPatch(value_count));
  }

  static std::size_t live_bytes();
  static std::size_t live_patches();
  static std::size_t allocations();
  static std::size_t releases();

private:
  std::unique_ptr<float[]> values_;
  std::size_t value_count_ = 0;

  static std::atomic<std::size_t> live_bytes_;
  static std::atomic<std::size_t> live_patches_;
  static std::atomic<std::size_t> allocations_;
  static std::atomic<std::size_t> releases_;
};

// FAST's existing visual key is the 0.5 m parent.  The local index is the
// eight 0.25 m subvoxels inside that parent, represented as xyz bits.
uint8_t VisualParentLocalIndex(const VOXEL_LOCATION &parent,
                               const V3D &position);

struct VisualParentHost
{
  std::vector<VisualPoint *> voxel_points;
  std::array<std::vector<VisualPoint *>, 8> local_attachments;
  int count = 0;

  explicit VisualParentHost(int initial_count = 0) : count(initial_count) {}
  ~VisualParentHost();

  VisualParentHost(const VisualParentHost &) = delete;
  VisualParentHost &operator=(const VisualParentHost &) = delete;

  void add(VisualPoint *point, uint8_t local_index);
};

using VOXEL_POINTS = VisualParentHost;
using VisualParentIndex =
    std::unordered_map<VOXEL_LOCATION, VisualParentHost *>;

struct VisualMemorySnapshot
{
  std::size_t parent_hosts = 0;
  std::size_t visual_points = 0;
  std::size_t features = 0;
  std::size_t observations = 0;
  std::size_t patch_bytes = 0;
  std::size_t vp_parent_max = 0;
  std::size_t feature_parent_max = 0;
  std::size_t patch_parent_max = 0;
};

// Parent-key owner registry matching Super's parent-key LRU semantics.  The
// lookup method is intentionally non-touching; only insertion/replacement
// may change LRU order.
class VisualParentRegistry
{
public:
  explicit VisualParentRegistry(std::size_t capacity = 1000000);
  ~VisualParentRegistry();

  VisualParentRegistry(const VisualParentRegistry &) = delete;
  VisualParentRegistry &operator=(const VisualParentRegistry &) = delete;

  VisualParentHost *findNoTouch(const VOXEL_LOCATION &key) const;
  VisualParentHost *getOrCreateForInsert(const VOXEL_LOCATION &key);
  void addPoint(const VOXEL_LOCATION &key, VisualPoint *point,
                uint8_t local_index);

  // The callback must remove any borrowed index before the owned host is
  // destroyed.  Host destruction then releases VisualPoints, Features,
  // patches, observations, and image references in that order.
  std::size_t evictToCapacity(
      const std::function<void(const VOXEL_LOCATION &)> &before_destroy);
  void clear(const std::function<void(const VOXEL_LOCATION &)> &before_destroy = {});

  std::size_t size() const { return owners_.size(); }
  std::size_t capacity() const { return capacity_; }
  void setCapacity(std::size_t capacity);
  std::vector<VOXEL_LOCATION> lruOrderForTest() const;
  VisualMemorySnapshot snapshot() const;

private:
  using OwnerMap = std::unordered_map<
      VOXEL_LOCATION, std::unique_ptr<VisualParentHost>>;

  std::size_t capacity_;
  OwnerMap owners_;
  std::list<VOXEL_LOCATION> lru_;
  std::unordered_map<VOXEL_LOCATION, std::list<VOXEL_LOCATION>::iterator>
      lru_positions_;

  void touch(const VOXEL_LOCATION &key);
};

VisualMemorySnapshot SnapshotVisualIndex(const VisualParentIndex &index);

#endif  // FAST_LIVO_VISUAL_MEMORY_H_
