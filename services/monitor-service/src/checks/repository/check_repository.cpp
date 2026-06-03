#include <checks/repository/check_repository.hpp>

#include <optional>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/storages/postgres/row.hpp>
#include <utility>

namespace netwatch::monitor_service::checks {
namespace {
CheckResult CheckResultFromRow(const userver::storages::postgres::Row& row) {
  return CheckResult{
      .id = row["id"].As<std::int64_t>(),
      .target_id = row["target_id"].As<std::int64_t>(),
      .status = CheckStatusFromString(row["status"].As<std::string>()),
      .protocol = CheckProtocolFromString(row["protocol"].As<std::string>()),
      .http_status = row["http_status"].As<std::optional<int> >(),
      .latency_ms = row["latency_ms"].As<std::optional<int> >(),
      .error_message = row["error_message"].As<std::optional<std::string> >(),
      .checked_at = row["checked_at"].As<std::string>(),
  };
}
}  // namespace

CheckRepository::CheckRepository(
    userver::storages::postgres::ClusterPtr pg_cluster)
    : pg_cluster_(std::move(pg_cluster)) {}

CheckResult CheckRepository::SaveCheckResult(const CheckResult& check) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            INSERT INTO check_results (
                target_id,
                status,
                protocol,
                http_status,
                latency_ms,
                error_message
            )
            VALUES ($1, $2, $3, $4, $5, $6)
            RETURNING
                id,
                target_id,
                status,
                protocol,
                http_status,
                latency_ms,
                error_message,
                to_char(checked_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS checked_at
        )",
      check.target_id, CheckStatusToString(check.status),
      CheckProtocolToString(check.protocol), check.http_status,
      check.latency_ms, check.error_message);

  return CheckResultFromRow(result.Front());
}

std::vector<CheckResult> CheckRepository::ListTargetChecks(
    std::int64_t target_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            SELECT
                id,
                target_id,
                status,
                protocol,
                http_status,
                latency_ms,
                error_message,
                to_char(checked_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS checked_at
            FROM check_results
            WHERE target_id = $1
            ORDER BY checked_at DESC, id DESC
        )",
      target_id);

  std::vector<CheckResult> checks;
  checks.reserve(result.Size());
  for (const auto& row : result) {
    checks.push_back(CheckResultFromRow(row));
  }

  return checks;
}

std::optional<CheckResult> CheckRepository::GetLatestTargetStatus(
    std::int64_t target_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            SELECT
                id,
                target_id,
                status,
                protocol,
                http_status,
                latency_ms,
                error_message,
                to_char(checked_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS checked_at
            FROM check_results
            WHERE target_id = $1
            ORDER BY checked_at DESC, id DESC
            LIMIT 1
        )",
      target_id);

  if (result.Size() == 0) {
    return std::nullopt;
  }

  return CheckResultFromRow(result.Front());
}

}  // namespace netwatch::monitor_service::checks
