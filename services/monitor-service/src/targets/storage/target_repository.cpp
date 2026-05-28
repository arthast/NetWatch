#include <targets/storage/target_repository.hpp>

#include <optional>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/storages/postgres/row.hpp>
#include <utility>
#include <vector>

#include <targets/model/target.hpp>

namespace monitor_service::target {
namespace {
Target TargetFromRow(const userver::storages::postgres::Row& row) {
  return Target{
      .id = row["id"].As<std::int64_t>(),
      .name = row["name"].As<std::string>(),
      .type = TargetTypeFromString(row["type"].As<std::string>()),
      .url = row["url"].As<std::optional<std::string> >(),
      .method = row["method"].As<std::optional<std::string> >(),
      .expected_status_code =
          row["expected_status_code"].As<std::optional<int> >(),
      .host = row["host"].As<std::optional<std::string> >(),
      .port = row["port"].As<std::optional<int> >(),
      .interval_seconds = row["interval_seconds"].As<int>(),
      .timeout_ms = row["timeout_ms"].As<int>(),
      .is_active = row["is_active"].As<bool>(),
  };
}
}  // namespace

TargetRepository::TargetRepository(
    userver::storages::postgres::ClusterPtr pg_cluster)
    : pg_cluster_(std::move(pg_cluster)) {}

Target TargetRepository::CreateTarget(
    const CreateTargetRequest& request) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            INSERT INTO targets (
                name,
                type,
                url,
                method,
                expected_status_code,
                host,
                port,
                interval_seconds,
                timeout_ms
            )
            VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9)
            RETURNING
                id,
                name,
                type,
                url,
                method,
                expected_status_code,
                host,
                port,
                interval_seconds,
                timeout_ms,
                is_active
        )",
      request.name, TargetTypeToString(request.type), request.url,
      request.method, request.expected_status_code, request.host, request.port,
      request.interval_seconds, request.timeout_ms);

  return TargetFromRow(result.Front());
}

std::vector<Target> TargetRepository::ListActiveTargets() const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            SELECT
                id,
                name,
                type,
                url,
                method,
                expected_status_code,
                host,
                port,
                interval_seconds,
                timeout_ms,
                is_active
            FROM targets
            WHERE is_active = TRUE
            ORDER BY id
        )");

  std::vector<Target> targets;
  targets.reserve(result.Size());
  for (const auto& row : result) {
    targets.push_back(TargetFromRow(row));
  }

  return targets;
}

std::optional<Target> TargetRepository::GetTargetById(
    std::int64_t target_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            SELECT
                id,
                name,
                type,
                url,
                method,
                expected_status_code,
                host,
                port,
                interval_seconds,
                timeout_ms,
                is_active
            FROM targets
            WHERE id = $1 AND is_active = TRUE
        )",
      target_id);

  if (result.Size() == 0) {
    return std::nullopt;
  }

  return TargetFromRow(result.Front());
}

std::optional<Target> TargetRepository::UpdateTarget(
    const Target& target) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            UPDATE targets
            SET
                name = $2,
                type = $3,
                url = $4,
                method = $5,
                expected_status_code = $6,
                host = $7,
                port = $8,
                interval_seconds = $9,
                timeout_ms = $10,
                updated_at = NOW()
            WHERE id = $1 AND is_active = TRUE
            RETURNING
                id,
                name,
                type,
                url,
                method,
                expected_status_code,
                host,
                port,
                interval_seconds,
                timeout_ms,
                is_active
        )",
      target.id, target.name, TargetTypeToString(target.type), target.url,
      target.method, target.expected_status_code, target.host, target.port,
      target.interval_seconds, target.timeout_ms);

  if (result.Size() == 0) {
    return std::nullopt;
  }

  return TargetFromRow(result.Front());
}

bool TargetRepository::DeactivateTarget(std::int64_t target_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            UPDATE targets
            SET is_active = FALSE,
                updated_at = NOW()
            WHERE id = $1 AND is_active = TRUE
            RETURNING id
        )",
      target_id);

  return result.Size() != 0;
}
}  // namespace monitor_service::target
