/* 
This file is part of FAST-LIVO2: Fast, Direct LiDAR-Inertial-Visual Odometry.

Developer: Chunran Zheng <zhengcr@connect.hku.hk>

For commercial use, please contact me at <zhengcr@connect.hku.hk> or
Prof. Fu Zhang at <fuzhang@hku.hk>.

This file is subject to the terms and conditions outlined in the 'LICENSE' file,
which is included as part of this source code package.
*/

#ifndef VIO_H_
#define VIO_H_

#include "voxel_map.h"
#include "feature.h"
#include "prob_livo/visual_plane_gate.h"
#include <opencv2/imgproc/imgproc_c.h>
#include <pcl/filters/voxel_grid.h>
#include <cstddef>
#include <functional>
#include <set>
#include <string>
#include <vikit/math_utils.h>
#include <vikit/robust_cost.h>
#include <vikit/vision.h>
#include <vikit/pinhole_camera.h>

struct SubSparseMap
{
  vector<float> propa_errors;
  vector<float> errors;
  vector<vector<float>> warp_patch;
  vector<int> search_levels;
  vector<VisualPoint *> voxel_points;
  vector<double> inv_expo_list;
  vector<pointWithVar> add_from_voxel_map;

  SubSparseMap()
  {
    propa_errors.reserve(SIZE_LARGE);
    errors.reserve(SIZE_LARGE);
    warp_patch.reserve(SIZE_LARGE);
    search_levels.reserve(SIZE_LARGE);
    voxel_points.reserve(SIZE_LARGE);
    inv_expo_list.reserve(SIZE_LARGE);
    add_from_voxel_map.reserve(SIZE_SMALL);
  };

  void reset()
  {
    propa_errors.clear();
    errors.clear();
    warp_patch.clear();
    search_levels.clear();
    voxel_points.clear();
    inv_expo_list.clear();
    add_from_voxel_map.clear();
  }
};

class Warp
{
public:
  Matrix2d A_cur_ref;
  int search_level;
  Warp(int level, Matrix2d warp_matrix) : search_level(level), A_cur_ref(warp_matrix) {}
  ~Warp() {}
};

class VOXEL_POINTS
{
public:
  std::vector<VisualPoint *> voxel_points;
  int count;
  VOXEL_POINTS(int num) : count(num) {}
  ~VOXEL_POINTS() 
  { 
    for (VisualPoint* vp : voxel_points) 
    {
      if (vp != nullptr) { delete vp; vp = nullptr; }
    }
  }
};

struct VisualPlaneQueryResult
{
  bool geometry_valid = false;
  bool uncertainty_valid = false;
  V3D normal_W = V3D::Zero();
  double d = 0.0;
  V3D center_W = V3D::Zero();
  double radius = 0.0;
  Eigen::Matrix4d plane_covariance_nd = Eigen::Matrix4d::Zero();
};

using VisualPlaneQuery = std::function<bool(
    const V3D &, VisualPlaneQueryResult &, std::string &)>;

struct VisualRuntimeCounters
{
  std::size_t camera_epochs = 0;
  std::size_t images_received = 0;
  std::size_t visual_process_calls = 0;
  std::size_t current_scan_candidates = 0;
  std::size_t current_scan_normal_valid = 0;
  std::size_t plane_queries = 0;
  std::size_t plane_geometry_valid = 0;
  std::size_t plane_uncertainty_valid = 0;
  std::size_t radius_gate_pass = 0;
  std::size_t radius_gate_reject = 0;
  std::size_t second_gate_pass = 0;
  std::size_t second_gate_reject = 0;
  std::size_t visual_points_created = 0;
  std::size_t reference_patch_update_attempts = 0;
  std::size_t reference_patch_updates_accepted = 0;
  std::size_t photometric_update_attempts = 0;
  std::size_t photometric_update_accepted = 0;
  std::size_t visual_state_commits = 0;
  std::size_t visual_rollbacks = 0;
};

class VIOManager
{
public:
  int grid_size;
  vk::AbstractCamera *cam;
  vk::PinholeCamera *pinhole_cam;
  StatesGroup *state;
  StatesGroup *state_propagat;
  M3D Rli, Rci, Rcl, Rcw, Jdphi_dR, Jdp_dt, Jdp_dR;
  V3D Pli, Pci, Pcl, Pcw;
  vector<int> grid_num;
  vector<int> map_index;
  vector<int> border_flag;
  vector<int> update_flag;
  vector<float> map_dist;
  vector<float> scan_value;
  vector<float> patch_buffer;
  bool normal_en, inverse_composition_en, exposure_estimate_en, raycast_en, has_ref_patch_cache;
  bool ncc_en = false, colmap_output_en = false;

  int width, height, grid_n_width, grid_n_height, length;
  double image_resize_factor;
  double fx, fy, cx, cy;
  int patch_pyrimid_level, patch_size, patch_size_total, patch_size_half, border, warp_len;
  int max_iterations, total_points;

  double img_point_cov, outlier_threshold, ncc_thre;
  
  SubSparseMap *visual_submap;
  std::vector<std::vector<V3D>> rays_with_sample_points;

  double compute_jacobian_time, update_ekf_time;
  double ave_total = 0;
  // double ave_build_residual_time = 0;
  // double ave_ekf_time = 0;

  int frame_count = 0;
  bool plot_flag;

  Matrix<double, DIM_STATE, DIM_STATE> G, H_T_H;
  MatrixXd K, H_sub_inv;

  ofstream fout_camera, fout_colmap;
  unordered_map<VOXEL_LOCATION, VOXEL_POINTS *> feat_map;
  unordered_map<VOXEL_LOCATION, int> sub_feat_map; 
  unordered_map<int, Warp *> warp_map;
  vector<VisualPoint *> retrieve_voxel_points;
  vector<pointWithVar> append_voxel_points;
  FramePtr new_frame_;
  cv::Mat img_cp, img_rgb, img_test;
  VisualRuntimeCounters visual_counters_;
  const VisualPlaneQuery *active_visual_plane_query_ = nullptr;
  prob_livo::VisualPlaneGateMode active_visual_gate_mode_ =
      prob_livo::VisualPlaneGateMode::kSuperLegacy;

  enum CellType
  {
    TYPE_MAP = 1,
    TYPE_POINTCLOUD,
    TYPE_UNKNOWN
  };

  VIOManager();
  ~VIOManager();
  void updateStateInverse(cv::Mat img, int level);
  void updateState(cv::Mat img, int level);
  void processFrame(cv::Mat &img, vector<pointWithVar> &pg, const unordered_map<VOXEL_LOCATION, VoxelOctoTree *> &feat_map, double img_time);
  void processFrame(cv::Mat &img, vector<pointWithVar> &pg,
                    const VisualPlaneQuery &plane_query,
                    prob_livo::VisualPlaneGateMode gate_mode,
                    double img_time);
  void retrieveFromVisualSparseMap(
      cv::Mat img, vector<pointWithVar> &pg,
      const unordered_map<VOXEL_LOCATION, VoxelOctoTree *> &plane_map,
      const VisualPlaneQuery *plane_query = nullptr);
  void generateVisualMapPoints(cv::Mat img, vector<pointWithVar> &pg);
  void setImuToLidarExtrinsic(const V3D &transl, const M3D &rot);
  void setLidarToCameraExtrinsic(vector<double> &R, vector<double> &P);
  void initializeVIO();
  void getImagePatch(cv::Mat img, V2D pc, float *patch_tmp, int level);
  void computeProjectionJacobian(V3D p, MD(2, 3) & J);
  void computeJacobianAndUpdateEKF(cv::Mat img);
  void resetGrid();
  void updateVisualMapPoints(cv::Mat img);
  void getWarpMatrixAffine(const vk::AbstractCamera &cam, const Vector2d &px_ref, const Vector3d &f_ref, const double depth_ref, const SE3 &T_cur_ref,
                           const int level_ref, 
                           const int pyramid_level, const int halfpatch_size, Matrix2d &A_cur_ref);
  void getWarpMatrixAffineHomography(const vk::AbstractCamera &cam, const V2D &px_ref,
                                     const V3D &xyz_ref, const V3D &normal_ref, const SE3 &T_cur_ref, const int level_ref, Matrix2d &A_cur_ref);
  void warpAffine(const Matrix2d &A_cur_ref, const cv::Mat &img_ref, const Vector2d &px_ref, const int level_ref, const int search_level,
                  const int pyramid_level, const int halfpatch_size, float *patch);
  void insertPointIntoVoxelMap(VisualPoint *pt_new);
  void plotTrackedPoints();
  void updateFrameState(StatesGroup state);
  void projectPatchFromRefToCur(const unordered_map<VOXEL_LOCATION, VoxelOctoTree *> &plane_map);
  void updateReferencePatch(
      const unordered_map<VOXEL_LOCATION, VoxelOctoTree *> &plane_map,
      const VisualPlaneQuery *plane_query = nullptr,
      prob_livo::VisualPlaneGateMode gate_mode =
          prob_livo::VisualPlaneGateMode::kSuperLegacy);
  void precomputeReferencePatches(int level);
  void dumpDataForColmap();
  double calculateNCC(float *ref_patch, float *cur_patch, int patch_size);
  int getBestSearchLevel(const Matrix2d &A_cur_ref, const int max_level);
  V3F getInterpolatedPixel(cv::Mat img, V2D pc);
  void recordImageReceived()
  {
    ++visual_counters_.images_received;
    ++visual_counters_.camera_epochs;
  }
  const VisualRuntimeCounters &visual_counters() const
  {
    return visual_counters_;
  }
  
  // void resetRvizDisplay();
  // deque<VisualPoint *> map_cur_frame;
  // deque<VisualPoint *> sub_map_ray;
  // deque<VisualPoint *> sub_map_ray_fov;
  // deque<VisualPoint *> visual_sub_map_cur;
  // deque<VisualPoint *> visual_converged_point;
  // std::vector<std::vector<V3D>> sample_points;

  // PointCloudXYZI::Ptr pg_down;
  // pcl::VoxelGrid<PointType> downSizeFilter;
};
typedef std::shared_ptr<VIOManager> VIOManagerPtr;

// Production-used preparation seam for FAST's current visual-map candidate
// path. A zero plane normal is the native skip condition; accepted candidates
// carry their world point and full visual covariance unchanged.
bool PrepareVisualMapCandidate(const pointWithVar &input,
                               pointWithVar &candidate);

#endif // VIO_H_
