#ifndef PROB_LIVO_SUPER_NATIVE_BASIC_ALIAS_H_
#define PROB_LIVO_SUPER_NATIVE_BASIC_ALIAS_H_

// P0-P4 copy-on-demand alias surface.  FAST-LIVO2 owns the ROS point
// registrations in preprocess.h; repeating Super-LIO's sensor registrations
// here would redefine the same PCL traits in the integrated translation unit.

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include <vector>

namespace BASIC {

using scalar = float;

using V2 = Eigen::Matrix<scalar, 2, 1>;
using V3 = Eigen::Matrix<scalar, 3, 1>;
using V4 = Eigen::Matrix<scalar, 4, 1>;
using V5 = Eigen::Matrix<scalar, 5, 1>;
using V6 = Eigen::Matrix<scalar, 6, 1>;
using V18 = Eigen::Matrix<scalar, 18, 1>;

using V2d = Eigen::Matrix<double, 2, 1>;
using V3d = Eigen::Matrix<double, 3, 1>;
using V4d = Eigen::Matrix<double, 4, 1>;
using V5d = Eigen::Matrix<double, 5, 1>;
using V6d = Eigen::Matrix<double, 6, 1>;

using VV3 = std::vector<V3, Eigen::aligned_allocator<V3>>;
using VV4 = std::vector<V4, Eigen::aligned_allocator<V4>>;
using VV5 = std::vector<V5, Eigen::aligned_allocator<V5>>;
using VV3d = std::vector<V3d, Eigen::aligned_allocator<V3d>>;

using M3 = Eigen::Matrix<scalar, 3, 3>;
using M4 = Eigen::Matrix<scalar, 4, 4>;
using M6 = Eigen::Matrix<scalar, 6, 6>;
using M3d = Eigen::Matrix<double, 3, 3>;
using M4d = Eigen::Matrix<double, 4, 4>;
using M6d = Eigen::Matrix<double, 6, 6>;

using Quat = Eigen::Quaternion<scalar>;
using Quatd = Eigen::Quaternion<double>;
using PointType = pcl::PointXYZI;
using PointCloudType = pcl::PointCloud<PointType>;
using CloudPtr = PointCloudType::Ptr;

}  // namespace BASIC

#endif  // PROB_LIVO_SUPER_NATIVE_BASIC_ALIAS_H_
