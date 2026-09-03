#include "prob_livo/prob_lio_backend.h"
#include "prob_livo/prob_plane_provider.h"
#include "test_i3_support.h"

#include <Eigen/Eigenvalues>

#include <chrono>
#include <cmath>
#include <string>

namespace {

using Map = LI2Sup::OctVoxMap<LI2Sup::V3, LI2Sup::scalar>;
using prob_livo::ProbLioBackend;
using prob_livo::ProbPlaneProvider;
using prob_livo::ProbPlaneQueryResult;
using prob_livo_test::MakeBackendEpoch;
using prob_livo_test::MakePlanePoints;
using prob_livo_test::TestContext;

LI2Sup::VV3 MakeTiltedSupport() {
  LI2Sup::VV3 points;
  points.emplace_back(0.0f, 0.0f, 2.0f);
  points.emplace_back(0.6f, 0.0f, 2.12f);
  points.emplace_back(0.0f, 0.6f, 1.94f);
  points.emplace_back(0.6f, 0.6f, 2.06f);
  points.emplace_back(0.3f, 0.3f, 2.08f);
  return points;
}

std::vector<Eigen::Matrix3d> MakeCovariances(std::size_t count) {
  std::vector<Eigen::Matrix3d> covariances(count);
  for (std::size_t index = 0; index < count; ++index) {
    covariances[index] = (0.001 + 0.0001 * index) *
                         Eigen::Matrix3d::Identity();
  }
  return covariances;
}

void CheckClose(TestContext &context, double error, double tolerance,
                const std::string &name) {
  context.Record(name, error);
  context.Check(error <= tolerance, name + " mismatch");
}

void FillDirect(const Map &map, const Eigen::Vector3d &query,
                Map::KNNHeapType &top_k) {
  map.getTopK(query.cast<float>(), top_k);
}

void TestProviderParityAndCenter(TestContext &context) {
  Map map(Map::Options{0.5f, 1000});
  const LI2Sup::VV3 points = MakeTiltedSupport();
  const std::vector<Eigen::Matrix3d> covariances =
      MakeCovariances(points.size());
  map.insert(points, covariances);
  const Eigen::Vector3d query(0.3, 0.3, 2.03);
  const ProbPlaneProvider provider(map);
  ProbPlaneQueryResult result;
  std::string error;
  const bool ok = provider.QueryAtWorldPoint(query, result, error);
  context.Check(ok && result.valid, "provider rejected valid tilted plane: " +
                                      error);
  if (!ok) return;

  Map::KNNHeapType direct;
  FillDirect(map, query, direct);
  context.Check(result.support_count == direct.count,
                "provider/direct support count mismatch");
  context.Check(result.support_points_W.size() == direct.count,
                "provider support point count mismatch");
  context.Check(result.support_covariances_W.size() == direct.count,
                "provider support covariance count mismatch");
  context.Check(result.support_ids.size() == direct.count,
                "provider support identity count mismatch");
  for (std::size_t index = 0; index < direct.count; ++index) {
    CheckClose(context,
               (result.support_points_W[index] -
                direct.points_[index].cast<double>())
                   .norm(),
               0.0, "parity.support_point");
    CheckClose(context,
               (result.support_covariances_W[index] - direct.covs_[index])
                   .cwiseAbs()
                   .maxCoeff(),
               0.0, "parity.support_covariance");
    context.Check(result.support_ids[index].voxel_key ==
                      direct.ids_[index].voxel_key &&
                      result.support_ids[index].local_index ==
                          direct.ids_[index].local_index,
                  "provider/direct support identity mismatch");
  }

  LI2Sup::PlanePointsArray direct_points;
  LI2Sup::PlaneCovsArray direct_covariances;
  for (int index = 0; index < direct.count; ++index) {
    direct_points[index] = direct.points_[index].cast<double>();
    direct_covariances[index] = direct.covs_[index];
  }
  const LI2Sup::PlaneFitQr fit =
      LI2Sup::SolvePlaneFitQr(direct_points, direct.count);
  const LI2Sup::ProbQrPlane direct_plane =
      LI2Sup::ComputeProbQrPlane(direct_points, direct_covariances,
                                 direct.count);
  context.Check(fit.solved && fit.legacy_accepted && fit.rank() == 3,
                "direct canonical QR fixture is invalid");
  context.Check(result.qr_rank == direct_plane.rank,
                "provider/direct QR rank mismatch");
  CheckClose(context, (result.coeff_nd - direct_plane.coeff).norm(), 0.0,
             "parity.qr_coefficients");
  CheckClose(context,
             (result.plane_cov_nd - direct_plane.covariance)
                 .cwiseAbs()
                 .maxCoeff(),
             0.0, "parity.qr_covariance");
  const double direct_residual_variance =
      LI2Sup::PlaneResidualVariance(query, direct_plane.covariance);
  Eigen::Vector4d query_h;
  query_h << query, 1.0;
  const double provider_residual_variance =
      query_h.dot(result.plane_cov_nd * query_h);
  CheckClose(context, provider_residual_variance - direct_residual_variance,
             0.0, "parity.query_residual_variance");

  Eigen::Vector3d support_mean = Eigen::Vector3d::Zero();
  for (const Eigen::Vector3d &point : result.support_points_W)
    support_mean += point;
  support_mean /= static_cast<double>(result.support_count);
  const Eigen::Vector3d expected_center =
      support_mean -
      (result.normal_W.dot(support_mean) + result.d) * result.normal_W;
  CheckClose(context, (result.center_W - expected_center).norm(), 0.0,
             "center.projection_formula");
  CheckClose(context, result.normal_W.dot(result.center_W) + result.d, 0.0,
             "center.plane_equation");
  CheckClose(context,
             (result.center_W - support_mean)
                 .cross(result.normal_W)
                 .norm(),
             1e-12, "center.normal_parallel_offset");
  context.Check((result.center_W - support_mean).norm() > 1e-6,
                "negative center=raw support centroid was not detectable");
  context.Check((result.center_W - query).norm() > 1e-6,
                "negative center=query point was not detectable");

  Eigen::Matrix3d support_covariance = Eigen::Matrix3d::Zero();
  for (const Eigen::Vector3d &point : result.support_points_W) {
    const Eigen::Vector3d delta = point - support_mean;
    support_covariance += delta * delta.transpose();
  }
  support_covariance /= static_cast<double>(result.support_count);
  Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eigensolver(
      support_covariance);
  const double expected_radius =
      std::sqrt(eigensolver.eigenvalues().maxCoeff());
  CheckClose(context, result.radius - expected_radius, 0.0,
             "radius.fast_support_spread");
}

void TestValidityAndReadOnly(TestContext &context) {
  Map map(Map::Options{0.5f, 1000});
  const LI2Sup::VV3 points = MakeTiltedSupport();
  map.insert(points, MakeCovariances(points.size()));
  const Eigen::Vector3d query(0.3, 0.3, 2.03);
  const ProbPlaneProvider provider(map);

  Map::KNNHeapType before;
  FillDirect(map, query, before);
  for (int iteration = 0; iteration < 3; ++iteration) {
    ProbPlaneQueryResult result;
    std::string error;
    context.Check(provider.QueryAtWorldPoint(query, result, error) &&
                      result.valid,
                  "read-only provider query failed: " + error);
  }
  Map::KNNHeapType after;
  FillDirect(map, query, after);
  context.Check(before.count == after.count,
                "provider query changed map support count");
  for (std::size_t index = 0; index < before.count; ++index) {
    context.Check(before.ids_[index].voxel_key == after.ids_[index].voxel_key &&
                      before.ids_[index].local_index ==
                          after.ids_[index].local_index,
                  "provider query changed map support identity");
    CheckClose(context,
               (before.points_[index] - after.points_[index]).norm(), 0.0,
               "readonly.support_point");
  }

  Map insufficient(Map::Options{0.5f, 1000});
  insufficient.insert({LI2Sup::V3(0.0f, 0.0f, 2.0f),
                        LI2Sup::V3(0.2f, 0.0f, 2.0f),
                        LI2Sup::V3(0.0f, 0.2f, 2.0f)},
                       MakeCovariances(3));
  const ProbPlaneProvider insufficient_provider(insufficient);
  ProbPlaneQueryResult insufficient_result;
  std::string insufficient_error;
  context.Check(!insufficient_provider.QueryAtWorldPoint(
                    Eigen::Vector3d(0.0, 0.0, 2.0), insufficient_result,
                    insufficient_error) &&
                    !insufficient_result.valid,
                "provider accepted fewer than four support points");

  Map degenerate(Map::Options{0.5f, 1000});
  degenerate.insert({LI2Sup::V3(0.0f, 0.0f, 2.0f),
                     LI2Sup::V3(0.2f, 0.0f, 2.0f),
                     LI2Sup::V3(0.4f, 0.0f, 2.0f),
                     LI2Sup::V3(0.6f, 0.0f, 2.0f),
                     LI2Sup::V3(0.8f, 0.0f, 2.0f)},
                    MakeCovariances(5));
  const ProbPlaneProvider degenerate_provider(degenerate);
  ProbPlaneQueryResult degenerate_result;
  std::string degenerate_error;
  context.Check(!degenerate_provider.QueryAtWorldPoint(
                    Eigen::Vector3d(0.4, 0.0, 2.0), degenerate_result,
                    degenerate_error) &&
                    !degenerate_result.valid,
                "provider accepted rank-deficient support");
}

void TestIntegrationSeamAndPerformance(TestContext &context) {
  StatesGroup state;
  ProbLioBackend::Options options;
  options.map_capacity = 1000;
  ProbLioBackend backend(state, options);
  const ProbPlaneProvider &backend_provider = backend.plane_provider();
  const std::vector<PointType> backend_points = MakePlanePoints();
  for (int index = 0; index < 54; ++index) {
    const double start = 0.01 * index;
    const auto imu = std::vector<prob_livo::ImuSample>{
        {start + 0.005, Eigen::Vector3d(0.0, 0.0, 9.7946),
         Eigen::Vector3d::Zero()}};
    LidarMeasureGroup packet = MakeBackendEpoch(
        start, start + 0.01, imu, backend_points);
    context.Check(backend.ProcessEpoch(packet),
                  "backend/provider integration fixture rejected an epoch");
  }
  context.Check(backend.lifecycle_state() == prob_livo::ProbLioLifecycle::RUN,
                "backend/provider integration fixture did not reach RUN");

  const StatesGroup state_before = state;
  const ProbLioBackend::Counters counters_before = backend.counters();
  ProbPlaneQueryResult backend_result;
  std::string backend_error;
  context.Check(backend_provider.QueryAtWorldPoint(
                    Eigen::Vector3d(0.0, 0.0, 2.0), backend_result,
                    backend_error) &&
                    backend_result.valid,
                "backend/provider ownership seam rejected populated map: " +
                    backend_error);
  for (int iteration = 0; iteration < 3; ++iteration) {
    ProbPlaneQueryResult repeated;
    std::string repeated_error;
    context.Check(backend_provider.QueryAtWorldPoint(
                      Eigen::Vector3d(0.0, 0.0, 2.0), repeated,
                      repeated_error) &&
                      repeated.valid,
                  "backend/provider repeated query failed: " + repeated_error);
  }
  CheckClose(context, (state.cov - state_before.cov).norm(), 0.0,
             "readonly.backend_state_covariance");
  CheckClose(context, (state.rot_end - state_before.rot_end).norm(), 0.0,
             "readonly.backend_state_rotation");
  CheckClose(context, (state.pos_end - state_before.pos_end).norm(), 0.0,
             "readonly.backend_state_position");
  context.Check(backend.counters().hknn_queries == counters_before.hknn_queries &&
                    backend.counters().qr_attempted ==
                        counters_before.qr_attempted &&
                    backend.counters().run_epochs == counters_before.run_epochs,
                "provider query changed LIO counters");

  Map map(Map::Options{0.5f, 1000});
  const LI2Sup::VV3 points = MakeTiltedSupport();
  map.insert(points, MakeCovariances(points.size()));
  const ProbPlaneProvider provider(map);
  const Eigen::Vector3d query(0.3, 0.3, 2.03);
  const auto start = std::chrono::steady_clock::now();
  int valid_queries = 0;
  for (int iteration = 0; iteration < 1000; ++iteration) {
    ProbPlaneQueryResult result;
    std::string error;
    if (provider.QueryAtWorldPoint(query, result, error) && result.valid)
      ++valid_queries;
  }
  const double elapsed_ms =
      std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - start)
          .count();
  context.Record("performance.1000_queries_ms", elapsed_ms);
  context.Check(valid_queries == 1000,
                "provider repeated query validity changed");
}

}  // namespace

int main() {
  TestContext context;
  TestProviderParityAndCenter(context);
  TestValidityAndReadOnly(context);
  TestIntegrationSeamAndPerformance(context);
  context.Print("G-I5 ProbPlaneProvider parity and read-only gates");
  return context.Passed() ? 0 : 1;
}
