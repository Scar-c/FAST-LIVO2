#include "feature.h"
#include "visual_memory.h"
#include "visual_point.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void Require(bool condition, const std::string &message)
{
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    std::exit(1);
  }
}

std::string ReadFile(const std::string &path)
{
  std::ifstream input(path);
  std::ostringstream contents;
  contents << input.rdbuf();
  return contents.str();
}

void TestUpdatePathUsesOwner()
{
  const std::string source = ReadFile(
      std::string(FAST_LIVO_SOURCE_DIR) + "/src/vio.cpp");
  const std::string begin = "void VIOManager::updateVisualMapPoints";
  const std::size_t begin_at = source.find(begin);
  const std::size_t end_at = source.find(
      "void VIOManager::updateReferencePatch", begin_at);
  Require(begin_at != std::string::npos && end_at != std::string::npos,
          "updateVisualMapPoints source contract is present");
  const std::string function = source.substr(begin_at, end_at - begin_at);
  Require(function.find("VisualPatch::Allocate") != std::string::npos,
          "temporary update patches use the RAII owner");
  Require(function.find("new float[patch_size_total]") == std::string::npos,
          "temporary update path has no raw patch allocation");
}

void TestPatchOwnershipAccounting()
{
  const std::size_t baseline = VisualPatch::live_bytes();
  {
    auto rejected = VisualPatch::Allocate(16);
    Require(VisualPatch::live_bytes() == baseline + 16 * sizeof(float),
            "rejected candidate owns its temporary patch");
  }
  Require(VisualPatch::live_bytes() == baseline,
          "rejected candidate releases its temporary patch");

  {
    auto *point = new VisualPoint(Vector3d::Zero());
    auto patch = VisualPatch::Allocate(16);
    Feature *feature = new Feature(point, std::move(patch), Vector2d::Zero(),
                                   Vector3d(0.0, 0.0, 1.0), SE3(), 0);
    point->addFrameRef(feature);
    Require(VisualPatch::live_bytes() == baseline + 16 * sizeof(float),
            "accepted candidate transfers patch ownership to Feature");
    delete point;
  }
  Require(VisualPatch::live_bytes() == baseline,
          "Feature destruction releases exactly one patch");

  for (int i = 0; i < 1000; ++i) {
    auto patch = VisualPatch::Allocate(8);
    VisualPoint point(Vector3d::Zero());
    point.addFrameRef(new Feature(&point, std::move(patch), Vector2d::Zero(),
                                  Vector3d(0.0, 0.0, 1.0), SE3(), 0));
  }
  Require(VisualPatch::live_bytes() == baseline,
          "repeated synthetic allocation/free accounting closes");
  Require(VisualPatch::live_patches() == 0,
          "no live patch survives the ownership tests");
}

void AddPoint(VisualParentRegistry &registry, const VOXEL_LOCATION &key,
              uint8_t local_index)
{
  auto *point = new VisualPoint(Vector3d::Zero());
  auto patch = VisualPatch::Allocate(4);
  point->addFrameRef(new Feature(point, std::move(patch), Vector2d::Zero(),
                                 Vector3d(0.0, 0.0, 1.0), SE3(), 0));
  registry.addPoint(key, point, local_index);
}

void TestParentLruLifecycle()
{
  VisualParentRegistry registry(2);
  const VOXEL_LOCATION a(0, 0, 0);
  const VOXEL_LOCATION b(1, 0, 0);
  const VOXEL_LOCATION c(2, 0, 0);
  for (uint8_t local = 0; local < 8; ++local) AddPoint(registry, a, local);
  AddPoint(registry, b, 0);
  registry.getOrCreateForInsert(a);  // replacement/touch, without new child
  const auto before_lookup = registry.lruOrderForTest();
  Require(registry.findNoTouch(a) != nullptr,
          "parent lookup finds a live host");
  Require(before_lookup == registry.lruOrderForTest(),
          "visual lookup does not touch parent LRU order");
  Require(registry.snapshot().visual_points == 9,
          "eight local subvoxels remain attached to one parent");

  AddPoint(registry, c, 0);
  Require(registry.lruOrderForTest().front() == c,
          "new parent is the LRU representative");
  registry.evictToCapacity({});
  Require(registry.findNoTouch(b) == nullptr,
          "least-recently-used parent is evicted");
  Require(registry.findNoTouch(a) != nullptr,
          "touched representative is not evicted");
  Require(registry.findNoTouch(c) != nullptr,
          "new representative survives replacement");
  Require(registry.snapshot().visual_points == 9,
          "eviction releases all child points of one parent together");

  const std::size_t live_before_clear = VisualPatch::live_bytes();
  Require(live_before_clear > 0, "LRU hosts retain their Feature patches");
  registry.clear({});
  Require(VisualPatch::live_bytes() == 0,
          "parent clear releases VisualPoints, Features, and patches");
  Require(registry.findNoTouch(a) == nullptr,
          "evicted host cannot be resurrected by a stale index");
}

}  // namespace

int main()
{
  TestUpdatePathUsesOwner();
  TestPatchOwnershipAccounting();
  TestParentLruLifecycle();
  std::cout << "PROMPT20 I7 visual memory tests passed\n";
  return 0;
}
