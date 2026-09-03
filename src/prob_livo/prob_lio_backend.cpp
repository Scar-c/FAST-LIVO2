#include "prob_livo/prob_lio_backend.h"

#include <Eigen/Geometry>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

#include <cmath>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <sstream>

namespace prob_livo {

namespace {

prob_livo::Options MakeFilterOptions(const ProbLioBackend::Options &options) {
  prob_livo::Options filter_options;
  filter_options.num_iterations = options.max_iterations;
  filter_options.quit_eps = options.quit_eps;
  filter_options.gravity_norm = options.gravity_norm;
  filter_options.gyro_variance = options.imu_gyro_variance;
  filter_options.accelerometer_variance = options.imu_accelerometer_variance;
  filter_options.gyro_bias_variance = options.imu_gyro_bias_variance;
  filter_options.accelerometer_bias_variance =
      options.imu_accelerometer_bias_variance;
  return filter_options;
}

ProbImuAdapter::Options MakeAdapterOptions(
    const ProbLioBackend::Options &options) {
  ProbImuAdapter::Options adapter_options;
  adapter_options.minimum_initialization_samples =
      options.minimum_initialization_samples;
  adapter_options.gravity_norm = options.gravity_norm;
  adapter_options.lidar_to_imu_rotation = options.lidar_to_imu_rotation;
  adapter_options.lidar_to_imu_translation = options.lidar_to_imu_translation;
  adapter_options.lidar_to_robot_yaw = options.lidar_to_robot_yaw;
  adapter_options.robot_origin = options.robot_origin;
  adapter_options.bridge_to_epoch_endpoint = !options.legacy_super_timing;
  adapter_options.time_tolerance = 2e-8;
  return adapter_options;
}

bool ToImuSample(const sensor_msgs::Imu::ConstPtr &message, ImuSample &sample) {
  if (message == nullptr) return false;
  sample.timestamp = message->header.stamp.toSec();
  sample.acceleration << message->linear_acceleration.x,
      message->linear_acceleration.y, message->linear_acceleration.z;
  sample.angular_velocity << message->angular_velocity.x,
      message->angular_velocity.y, message->angular_velocity.z;
  return std::isfinite(sample.timestamp) && sample.acceleration.allFinite() &&
         sample.angular_velocity.allFinite();
}

}  // namespace

ProbLioBackend::ProbLioBackend(StatesGroup &state, const Options &options)
    : state_(state),
      options_(options),
      filter_(state_, MakeFilterOptions(options)),
      imu_adapter_(MakeAdapterOptions(options)),
      map_(new LI2Sup::OctVoxMap<LI2Sup::V3, LI2Sup::scalar>(
          LI2Sup::OctVoxMap<LI2Sup::V3, LI2Sup::scalar>::Options{
              static_cast<float>(options.map_resolution), options.map_capacity})),
      plane_provider_(*map_) {
  map_->SetCovStoragePrecision(options_.cov_storage_precision);
  downsample_filter_.setLeafSize(static_cast<float>(options_.voxel_size));

  if (!options_.trajectory_path.empty()) {
    const std::filesystem::path path(options_.trajectory_path);
    std::error_code error;
    if (path.has_parent_path()) {
      std::filesystem::create_directories(path.parent_path(), error);
    }
    trajectory_.open(path, std::ios::out | std::ios::trunc);
    if (!trajectory_.is_open()) {
      SetError("cannot open Prob-LIO trajectory: " + options_.trajectory_path);
    }
  }
}

ProbLioBackend::~ProbLioBackend() {
  if (!options_.trajectory_path.empty()) {
    std::ofstream counters(options_.trajectory_path + ".counters.yaml",
                            std::ios::out | std::ios::trunc);
    if (counters.is_open()) {
      counters << "schema_version: 3\n"
               << "successful_epochs: " << counters_.successful_epochs << "\n"
               << "imu_init_epochs: " << counters_.imu_init_epochs << "\n"
               << "map_init_epochs: " << counters_.map_init_epochs << "\n"
               << "run_epochs: " << counters_.run_epochs << "\n"
               << "map_init_inserts: " << counters_.map_init_inserts << "\n"
               << "map_update_inserts: " << counters_.map_update_inserts << "\n"
               << "undistorted_points: " << counters_.undistorted_points << "\n"
               << "downsampled_points: " << counters_.downsampled_points << "\n"
               << "hknn_queries: " << counters_.hknn_queries << "\n"
               << "hknn_returns: " << counters_.hknn_returns << "\n"
               << "qr_attempted: " << counters_.qr_attempted << "\n"
               << "qr_valid: " << counters_.qr_valid << "\n"
               << "weighted_measurements: " << counters_.weighted_measurements
               << "\n"
               << "legacy_measurements: " << counters_.legacy_measurements
               << "\n"
               << "lidar_callbacks_received: "
               << counters_.lidar_callbacks_received << "\n"
               << "scheduler_epochs_emitted: "
               << counters_.scheduler_epochs_emitted << "\n"
               << "scheduler_lidar_pending_at_shutdown: "
               << counters_.scheduler_lidar_pending_at_shutdown << "\n"
               << "scheduler_lidar_discarded: "
               << counters_.scheduler_lidar_discarded << "\n"
               << "backend_epochs_attempted: "
               << counters_.backend_epochs_attempted << "\n"
               << "backend_epochs_success: "
               << counters_.backend_epochs_success << "\n"
               << "backend_epochs_rejected: "
               << counters_.backend_epochs_rejected << "\n"
               << "trajectory_rows: " << counters_.trajectory_rows << "\n"
               << "adapted_scans: " << counters_.adapted_scans << "\n"
               << "adapted_points: " << counters_.adapted_points << "\n";
    }
  }
  if (trajectory_.is_open()) trajectory_.close();
}

bool ProbLioBackend::ProcessEpoch(LidarMeasureGroup &measures,
                                  SchedulerMode mode) {
  ++counters_.backend_epochs_attempted;
  last_error_.clear();
  const auto reject = [this](const std::string &message) {
    SetError(message);
    ++counters_.backend_epochs_rejected;
    return false;
  };
  const bool livo_mode = mode == SchedulerMode::kLivo;
  if (measures.measures.empty() || measures.pcl_proc_cur == nullptr ||
      (!livo_mode && measures.lidar == nullptr)) {
    return reject("Prob-LIO scheduler packet is incomplete");
  }
  const double epoch_end = measures.measures.back().lio_time;
  if (!std::isfinite(epoch_end) || epoch_end < 0.0) {
    return reject("Prob-LIO scheduler endpoint is invalid");
  }

  if (!anchor_seeded_) {
    if (!lifecycle_.SeedSchedulerAnchor(measures)) {
      return reject("cannot seed the Prob-LIO scheduler epoch anchor");
    }
    anchor_seeded_ = true;
  }
  if (std::abs(measures.last_lio_update_time -
               lifecycle_.scheduler_epoch_anchor()) > 2e-8) {
    return reject("scheduler epoch anchor was mutated outside lifecycle authority");
  }

  bool success = false;
  switch (lifecycle_.lifecycle()) {
    case ProbLioLifecycle::IMU_INIT:
      success = ProcessImuInit(measures, epoch_end, mode);
      break;
    case ProbLioLifecycle::MAP_INIT:
      success = ProcessMapInit(measures, epoch_end, mode);
      break;
    case ProbLioLifecycle::RUN:
      success = ProcessRun(measures, epoch_end, mode);
      break;
    default:
      return reject("unknown Prob-LIO lifecycle state");
  }
  if (success) {
    ++counters_.backend_epochs_success;
  } else {
    ++counters_.backend_epochs_rejected;
  }
  return success;
}

bool ProbLioBackend::ProcessImuInit(LidarMeasureGroup &measures,
                                    double epoch_end, SchedulerMode mode) {
  const ProbImuAdapter::Result result = imu_adapter_.ProcessLioEpoch(
      measures, filter_, mode);
  if (!result.success) {
    SetError(result.message);
    return false;
  }
  if (!lifecycle_.CommitConsumedEpoch(measures, epoch_end)) {
    SetError("failed to commit the IMU-init scheduler epoch");
    return false;
  }
  ++counters_.successful_epochs;
  ++counters_.imu_init_epochs;
  if (result.initialized && !lifecycle_.MarkFilterInitialized()) {
    SetError("failed to hand off from IMU_INIT to MAP_INIT");
    return false;
  }
  return true;
}

bool ProbLioBackend::ProcessMapInit(LidarMeasureGroup &measures,
                                    double epoch_end, SchedulerMode mode) {
  const PointCloudXYZI::Ptr &map_scan =
      mode == SchedulerMode::kLivo ? measures.pcl_proc_cur : measures.lidar;
  if (map_scan == nullptr || !InsertInitialMap(*map_scan)) return false;
  // Super map_init() advances last_obs_time without propagating the filter.
  // The first RUN epoch then bridges from this boundary with accepted IMU.
  filter_.SetLastObservationTime(epoch_end);
  if (!lifecycle_.CommitConsumedEpoch(measures, epoch_end)) {
    SetError("failed to commit the map-init scheduler epoch");
    return false;
  }
  ++counters_.successful_epochs;
  ++counters_.map_init_epochs;
  ++map_init_scan_count_;
  if (map_init_scan_count_ >= options_.map_initialization_scans &&
      !lifecycle_.MarkMapInitialized()) {
    SetError("failed to hand off from MAP_INIT to RUN");
    return false;
  }
  return true;
}

bool ProbLioBackend::ConvertLookahead(
    const sensor_msgs::Imu::ConstPtr &message, ImuSample &sample) const {
  return ToImuSample(message, sample);
}

bool ProbLioBackend::ProcessRun(LidarMeasureGroup &measures, double epoch_end,
                                SchedulerMode mode) {
  ImuSample lookahead;
  const ImuSample *lookahead_ptr = nullptr;
  if (measures.imu_lookahead != nullptr && !options_.legacy_super_timing) {
    if (!ConvertLookahead(measures.imu_lookahead, lookahead)) {
      SetError("scheduler supplied an invalid Prob-LIO IMU look-ahead");
      return false;
    }
    lookahead_ptr = &lookahead;
  }

  const ProbImuAdapter::Result result = imu_adapter_.ProcessLioEpoch(
      measures, filter_, mode, lookahead_ptr);
  if (!result.success) {
    SetError(result.message);
    return false;
  }
  undistorted_scan_ = result.prob_scan_undistort_imu;
  counters_.undistorted_points += undistorted_scan_->size();

  if (!BuildAndSolveScan()) return false;
  std::string adapter_error;
  if (!point_with_var_adapter_.Build(
          static_cast<std::uint64_t>(counters_.run_epochs), *downsampled_scan_,
          points_body_, lidar_covariances_, body_covariances_, state_.rot_end,
          state_.pos_end, state_.cov, options_.lidar_to_imu_rotation,
          options_.lidar_to_imu_translation, effect_mask_,
          plane_coefficients_, adapter_error)) {
    SetError("Prob-LIO pointWithVar adapter failed: " + adapter_error);
    return false;
  }
  ++counters_.adapted_scans;
  counters_.adapted_points += point_with_var_adapter_.output().points.size();
  UpdateMap();
  BuildWorldScan();

  if (!lifecycle_.CommitConsumedEpoch(measures, epoch_end)) {
    SetError("failed to commit the RUN scheduler epoch");
    return false;
  }
  ++counters_.successful_epochs;
  ++counters_.run_epochs;
  if (!options_.defer_trajectory_until_camera_epoch) {
    AppendTrajectory(options_.legacy_super_timing ? filter_.current_time()
                                                  : epoch_end);
  }
  return true;
}

bool ProbLioBackend::InsertInitialMap(const PointCloudXYZI &raw_scan) {
  const std::size_t point_count = raw_scan.points.size();
  LI2Sup::VV3 points_lidar;
  points_lidar.resize(point_count);
  LI2Sup::VV3 points_world;
  points_world.resize(point_count);
  tbb::parallel_for(
      tbb::blocked_range<std::size_t>(0, point_count),
      [&](const tbb::blocked_range<std::size_t> &range) {
        for (std::size_t index = range.begin(); index < range.end(); ++index) {
          const PointType &point = raw_scan.points[index];
          points_lidar[index] = LI2Sup::V3(point.x, point.y, point.z);
          const Eigen::Vector3d point_imu =
              options_.lidar_to_imu_rotation *
                  points_lidar[index].cast<double>() +
              options_.lidar_to_imu_translation;
          points_world[index] =
              (state_.rot_end * point_imu + state_.pos_end).cast<float>();
        }
      });

  if (options_.covariance_pipeline_enabled) {
    LI2Sup::ComputeInitMapCovList(
        points_lidar, options_.lidar_to_imu_rotation,
        options_.lidar_to_imu_translation, options_.lidar_depth_error,
        options_.lidar_beam_error_deg, state_.rot_end,
        state_.cov.block<3, 3>(Layout::kRot0, Layout::kRot0),
        state_.cov.block<3, 3>(Layout::kPos0, Layout::kPos0),
        map_covariances_, options_.map_pose_cov_model);
    for (const auto &covariance : map_covariances_) {
      if (!LI2Sup::ValidateCovariance(
              covariance, options_.covariance_validation_mode,
              options_.map_covariance_validation_tolerance)) {
        SetError("initial map covariance failed validation");
        return false;
      }
    }
    map_->insert(points_world, map_covariances_);
  } else {
    map_->insert(points_world);
  }
  counters_.map_init_inserts += point_count;
  return true;
}

bool ProbLioBackend::BuildAndSolveScan() {
  if (undistorted_scan_ == nullptr) {
    SetError("Prob-LIO has no undistorted scan");
    return false;
  }
  downsample_filter_.setInputCloud(undistorted_scan_);
  downsample_filter_.filter(downsampled_scan_);
  counters_.downsampled_points += downsampled_scan_->size();

  points_body_.resize(downsampled_scan_->size());
  tbb::parallel_for(
      tbb::blocked_range<std::size_t>(0, downsampled_scan_->size()),
      [&](const tbb::blocked_range<std::size_t> &range) {
        for (std::size_t index = range.begin(); index < range.end(); ++index) {
          const PointType &point = downsampled_scan_->points[index];
          points_body_[index] = LI2Sup::V3(point.x, point.y, point.z);
        }
      });
  if (options_.covariance_pipeline_enabled) {
    LI2Sup::ComputeBodyCovListWithExtrinsic(
        points_body_, options_.lidar_to_imu_rotation,
        options_.lidar_to_imu_translation, options_.lidar_depth_error,
        options_.lidar_beam_error_deg, body_covariances_,
        &lidar_covariances_);
    for (const auto &covariance : body_covariances_) {
      if (!LI2Sup::ValidateCovariance(
              covariance, options_.covariance_validation_mode,
              options_.map_covariance_validation_tolerance)) {
        SetError("body covariance failed validation");
        return false;
      }
    }
  } else {
    body_covariances_.clear();
  }

  plane_covariances_.assign(points_body_.size(), LI2Sup::ProbQrPlane());
  plane_coefficients_.assign(points_body_.size(), Eigen::Vector4d::Zero());
  effect_mask_.assign(points_body_.size(), false);

  const bool update_ok = filter_.UpdateObserve(
      [this](const StatesGroup &state, bool need_converge,
             Matrix6 &hth, Vector6 &htr) {
        std::vector<ObservationContribution, Eigen::aligned_allocator<
            ObservationContribution>> contributions(points_body_.size());

        // Association and QR are read-only with respect to the map. Each
        // point owns one contribution slot; only the ordered reduction below
        // touches the ESKF accumulator and scalar counters.
        tbb::parallel_for(
            tbb::blocked_range<std::size_t>(0, points_body_.size()),
            [&](const tbb::blocked_range<std::size_t> &range) {
              for (std::size_t index = range.begin(); index < range.end();
                   ++index) {
                ObservationContribution &contribution = contributions[index];
                const LI2Sup::V3 point_world_float =
                    (state.rot_end * points_body_[index].cast<double>() +
                     state.pos_end)
                        .cast<float>();
                const LI2Sup::V3d point_world =
                    point_world_float.cast<double>();

                if (!need_converge) {
                  LI2Sup::OctVoxMap<LI2Sup::V3,
                                    LI2Sup::scalar>::KNNHeapType top_k;
                  map_->getTopK(point_world_float, top_k);
                  contribution.hknn_queried = true;
                  contribution.hknn_returns = top_k.count;
                  if (top_k.count < 4) {
                    effect_mask_[index] = false;
                    continue;
                  }

                  LI2Sup::PlanePointsArray plane_points;
                  LI2Sup::PlaneCovsArray plane_covariances;
                  for (int neighbor = 0; neighbor < top_k.count; ++neighbor) {
                    plane_points[neighbor] =
                        top_k.points_[neighbor].cast<double>();
                    plane_covariances[neighbor] = top_k.covs_[neighbor];
                  }
                  const LI2Sup::PlaneFitQr fit =
                      LI2Sup::SolvePlaneFitQr(plane_points, top_k.count);
                  effect_mask_[index] = fit.solved && fit.legacy_accepted;
                  if (!effect_mask_[index]) continue;
                  const double scale = fit.q.norm();
                  plane_coefficients_[index] << fit.q / scale, 1.0 / scale;

                  if (options_.qr_plane_covariance_enabled ||
                      options_.p2p_weight_mode ==
                          LI2Sup::P2pWeightMode::ProbLivo2) {
                    contribution.qr_attempted = true;
                    plane_covariances_[index] = LI2Sup::ComputeProbQrPlane(
                        plane_points, plane_covariances, top_k.count);
                    contribution.qr_valid =
                        plane_covariances_[index].status ==
                        LI2Sup::ProbQrPlane::kValid;
                  }
                }

                if (!effect_mask_[index]) continue;
                const Eigen::Vector4d &plane = plane_coefficients_[index];
                const double residual =
                    plane.head<3>().dot(point_world) + plane[3];
                const double length = points_body_[index].cast<double>().norm();
                if (!(length > 81.0 * residual * residual)) {
                  effect_mask_[index] = false;
                  continue;
                }

                const Eigen::Vector3d normal = plane.head<3>();
                const Eigen::Vector3d normal_body =
                    state.rot_end.transpose() * normal;
                const Eigen::Vector3d point_body =
                    points_body_[index].cast<double>();
                Vector6 jacobian;
                jacobian.head<3>() = point_body.cross(normal_body);
                jacobian.tail<3>() = normal;

                double weight = 1000.0;
                if (options_.p2p_weight_mode ==
                    LI2Sup::P2pWeightMode::ProbLivo2) {
                  const LI2Sup::ProbQrPlane &qr = plane_covariances_[index];
                  if (qr.status != LI2Sup::ProbQrPlane::kValid) continue;
                  const double plane_variance =
                      LI2Sup::PlaneResidualVariance(point_world, qr.covariance);
                  const double point_variance =
                      options_.covariance_pipeline_enabled
                          ? LI2Sup::PointResidualVariance(
                                normal, state.rot_end,
                                body_covariances_[index])
                          : 0.0;
                  const LI2Sup::ProbWeight probability_weight =
                      LI2Sup::ComputeP2pProbWeight(plane_variance,
                                                   point_variance);
                  if (!probability_weight.valid) continue;
                  weight = probability_weight.weight;
                  contribution.weighted = true;
                } else {
                  contribution.legacy = true;
                }
                contribution.hth =
                    jacobian * weight * jacobian.transpose();
                contribution.htr = -jacobian * weight * residual;
              }
            });

        // Keep the original point-index order for floating-point accumulation,
        // so parallel scheduling changes throughput but not the reduction
        // contract used by the online and offline paths.
        for (const ObservationContribution &contribution : contributions) {
          hth += contribution.hth;
          htr += contribution.htr;
          if (contribution.hknn_queried) {
            ++counters_.hknn_queries;
            counters_.hknn_returns += contribution.hknn_returns;
          }
          if (contribution.qr_attempted) ++counters_.qr_attempted;
          if (contribution.qr_valid) ++counters_.qr_valid;
          if (contribution.weighted) ++counters_.weighted_measurements;
          if (contribution.legacy) ++counters_.legacy_measurements;
        }
      });
  if (!update_ok) {
    SetError("ProbESKF19 observation update failed");
    return false;
  }
  return true;
}

void ProbLioBackend::UpdateMap() {
  const std::size_t point_count = points_body_.size();
  if (point_count == 0) return;
  LI2Sup::VV3 points_world;
  points_world.resize(point_count);
  tbb::parallel_for(
      tbb::blocked_range<std::size_t>(0, point_count),
      [&](const tbb::blocked_range<std::size_t> &range) {
        for (std::size_t index = range.begin(); index < range.end(); ++index) {
          points_world[index] =
              (state_.rot_end * points_body_[index].cast<double>() +
               state_.pos_end)
                  .cast<float>();
        }
      });
  if (options_.covariance_pipeline_enabled) {
    LI2Sup::ComputeMapCovList(
        points_body_, body_covariances_, state_.rot_end,
        state_.cov.block<3, 3>(Layout::kRot0, Layout::kRot0),
        state_.cov.block<3, 3>(Layout::kPos0, Layout::kPos0),
        map_covariances_, options_.map_pose_cov_model);
    for (const auto &covariance : map_covariances_) {
      if (!LI2Sup::ValidateCovariance(
              covariance, options_.covariance_validation_mode,
              options_.map_covariance_validation_tolerance)) {
        SetError("map-update covariance failed validation");
        return;
      }
    }
    map_->insert(points_world, map_covariances_);
  } else {
    map_->insert(points_world);
  }
  counters_.map_update_inserts += point_count;
}

void ProbLioBackend::BuildWorldScan() {
  world_scan_->clear();
  if (undistorted_scan_ == nullptr) return;
  world_scan_->resize(undistorted_scan_->size());
  tbb::parallel_for(
      tbb::blocked_range<std::size_t>(0, undistorted_scan_->size()),
      [&](const tbb::blocked_range<std::size_t> &range) {
        for (std::size_t index = range.begin(); index < range.end(); ++index) {
          const PointType &input = undistorted_scan_->points[index];
          PointType output = input;
          const Eigen::Vector3d world =
              state_.rot_end * Eigen::Vector3d(input.x, input.y, input.z) +
              state_.pos_end;
          output.x = static_cast<float>(world.x());
          output.y = static_cast<float>(world.y());
          output.z = static_cast<float>(world.z());
          world_scan_->points[index] = output;
        }
      });
  world_scan_->header = undistorted_scan_->header;
  world_scan_->is_dense = undistorted_scan_->is_dense;
}

void ProbLioBackend::AppendTrajectory(double timestamp) {
  if (!trajectory_.is_open()) return;
  Eigen::Quaterniond quaternion(state_.rot_end);
  quaternion.normalize();
  trajectory_ << std::setprecision(17) << timestamp << " " << state_.pos_end.x()
              << " " << state_.pos_end.y() << " " << state_.pos_end.z() << " "
              << quaternion.x() << " " << quaternion.y() << " "
              << quaternion.z() << " " << quaternion.w() << "\n";
  ++counters_.trajectory_rows;
  trajectory_.flush();
}

}  // namespace prob_livo
