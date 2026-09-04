#include "voxel_map.h"

#include <cstring>
#include <new>

int main()
{
  alignas(PointToPlane) unsigned char storage[sizeof(PointToPlane)];
  std::memset(storage, 0xA8, sizeof(storage));
  PointToPlane *point = new (storage) PointToPlane;
  const bool valid = point->is_valid_;
  point->~PointToPlane();
  return valid ? 1 : 0;
}
