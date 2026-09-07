#include "prob_livo/super_native/OctVoxMap/OctVoxMap.hpp"
#include "feature.h"
#include "visual_memory.h"
#include "visual_point.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Require(bool condition, const std::string &message)
{
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    std::exit(1);
  }
}

void AddVisualParent(VisualParentRegistry &registry,
                     std::unordered_map<VOXEL_LOCATION, VisualParentHost *> &index,
                     const VOXEL_LOCATION &key)
{
  auto *point = new VisualPoint(Vector3d::Zero());
  auto patch = VisualPatch::Allocate(4);
  point->addFrameRef(new Feature(point, std::move(patch), Vector2d::Zero(),
                                 Vector3d(0.0, 0.0, 1.0), SE3(), 0));
  VisualParentHost *host = registry.getOrCreate(key);
  host->add(point, 0);
  index[key] = host;
}

std::string ReadFile(const std::string &path)
{
  std::ifstream input(path);
  std::ostringstream contents;
  contents << input.rdbuf();
  return contents.str();
}

void TestNoIndependentVisualVictimPolicy()
{
  const std::string header = ReadFile(
      std::string(FAST_LIVO_SOURCE_DIR) + "/include/visual_memory.h");
  const std::string implementation = ReadFile(
      std::string(FAST_LIVO_SOURCE_DIR) + "/src/visual_memory.cpp");
  Require(header.find("lru_") == std::string::npos &&
              header.find("capacity_") == std::string::npos &&
              header.find("evictToCapacity") == std::string::npos &&
              implementation.find("touch(") == std::string::npos,
          "visual owner has no independent eviction policy");
}

void TestRealGeometryEvictionBridge()
{
  using Map = LI2Sup::OctVoxMap<LI2Sup::V3, LI2Sup::scalar>;
  Map map(Map::Options{0.5f, 3});
  VisualParentRegistry registry;
  std::unordered_map<VOXEL_LOCATION, VisualParentHost *> visual_index;
  std::vector<Eigen::Vector3i> geometry_victims;

  const VOXEL_LOCATION a(0, 0, 0);
  const VOXEL_LOCATION b(1, 0, 0);
  const VOXEL_LOCATION c(2, 0, 0);
  const VOXEL_LOCATION d(3, 0, 0);
  AddVisualParent(registry, visual_index, a);
  AddVisualParent(registry, visual_index, b);
  AddVisualParent(registry, visual_index, c);

  map.SetParentEvictionCallback([&](const Eigen::Vector3i &key) {
    geometry_victims.push_back(key);
    const VOXEL_LOCATION visual_key(key.x(), key.y(), key.z());
    visual_index.erase(visual_key);
    Require(registry.eraseBySuperEviction(visual_key),
            "actual geometry victim erases the same visual host");
  });

  Map::Points a_cloud;
  a_cloud.emplace_back(0.01f, 0.01f, 0.01f);
  map.insert(a_cloud);
  Map::Points b_cloud;
  b_cloud.emplace_back(0.51f, 0.01f, 0.01f);
  map.insert(b_cloud);

  // Reinsert A through the real geometry map to establish [A, B] recency.
  // Visual-only lookup and insertion are deliberately performed before C;
  // they must not alter the geometry victim order.
  Require(registry.findNoTouch(a) != nullptr,
          "visual-only lookup finds A without geometry access");
  AddVisualParent(registry, visual_index, a);
  map.insert(a_cloud);

  Map::Points c_cloud;
  c_cloud.emplace_back(1.01f, 0.01f, 0.01f);
  map.insert(c_cloud);
  Require(geometry_victims.size() == 1,
          "one geometry insertion causes exactly one callback");
  Require(geometry_victims.front() == Eigen::Vector3i(1, 0, 0),
          "geometry LRU selects B as the victim");
  Require(registry.findNoTouch(b) == nullptr &&
              registry.findNoTouch(a) != nullptr,
          "same-key visual erase removes B and preserves A");
  Require(visual_index.find(b) == visual_index.end(),
          "borrowed visual index is erased before host destruction");

  AddVisualParent(registry, visual_index, d);
  Map::Points d_cloud;
  d_cloud.emplace_back(1.51f, 0.01f, 0.01f);
  std::vector<Eigen::Matrix3d> covariance(1, Eigen::Matrix3d::Identity());
  map.insert(d_cloud, covariance);
  Require(geometry_victims.size() == 2,
          "covariance insertion path also invokes the callback once");
  Require(geometry_victims.back() == Eigen::Vector3i(0, 0, 0),
          "second real geometry path reports its exact victim key");
  Require(registry.findNoTouch(a) == nullptr &&
              registry.findNoTouch(c) != nullptr &&
              registry.findNoTouch(d) != nullptr,
          "non-victim visual hosts survive geometry eviction");
  Require(VisualPatch::live_bytes() > 0,
          "remaining geometry parents retain their visual children");

  registry.clear({});
  Require(VisualPatch::live_bytes() == 0,
          "clear releases all remaining visual child state");
}

}  // namespace

int main()
{
  TestNoIndependentVisualVictimPolicy();
  TestRealGeometryEvictionBridge();
  std::cout << "PROMPT21 real geometry parent eviction tests passed\n";
  return 0;
}
