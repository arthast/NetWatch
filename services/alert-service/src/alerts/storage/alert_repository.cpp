#include <alerts/storage/alert_repository.hpp>

#include <optional>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/storages/postgres/row.hpp>
#include <utility>

namespace netwatch::alert_service::alerts {
namespace {

Alert AlertFromRow(const userver::storages::postgres::Row& row) {
  return Alert{
      .id = row["id"].As<std::int64_t>(),
      .target_id = row["target_id"].As<std::int64_t>(),
      .type = AlertTypeFromString(row["type"].As<std::string>()),
      .severity = AlertSeverityFromString(row["severity"].As<std::string>()),
      .message = row["message"].As<std::string>(),
      .created_at = row["created_at"].As<std::string>(),
      .resolved_at = row["resolved_at"].As<std::optional<std::string>>(),
  };
}

constexpr std::string_view kAlertFields = R"(
    id,
    target_id,
    type,
    severity,
    message,
    to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS created_at,
    to_char(resolved_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS resolved_at
)";

}  // namespace

AlertRepository::AlertRepository(
    userver::storages::postgres::ClusterPtr pg_cluster)
    : pg_cluster_(std::move(pg_cluster)) {}

Alert AlertRepository::CreateAlert(const NewAlert& alert) const {
  const auto result =
      pg_cluster_->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          R"(
            INSERT INTO alerts (
                target_id,
                type,
                severity,
                message
            )
            VALUES ($1, $2, $3, $4)
            RETURNING
        )" + std::string{kAlertFields},
          alert.target_id, AlertTypeToString(alert.type),
          AlertSeverityToString(alert.severity), alert.message);

  return AlertFromRow(result.Front());
}

std::optional<Alert> AlertRepository::FindActiveAlert(std::int64_t target_id,
                                                      AlertType type) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            SELECT
        )" +
          std::string{kAlertFields} + R"(
            FROM alerts
            WHERE target_id = $1
              AND type = $2
              AND resolved_at IS NULL
            ORDER BY created_at DESC, id DESC
            LIMIT 1
        )",
      target_id, AlertTypeToString(type));

  if (result.Size() == 0) {
    return std::nullopt;
  }

  return AlertFromRow(result.Front());
}

std::optional<Alert> AlertRepository::ResolveActiveAlert(std::int64_t target_id,
                                                         AlertType type) const {
  const auto result =
      pg_cluster_->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          R"(
            UPDATE alerts
            SET resolved_at = NOW()
            WHERE id = (
                SELECT id
                FROM alerts
                WHERE target_id = $1
                  AND type = $2
                  AND resolved_at IS NULL
                ORDER BY created_at DESC, id DESC
                LIMIT 1
            )
            RETURNING
        )" + std::string{kAlertFields},
          target_id, AlertTypeToString(type));

  if (result.Size() == 0) {
    return std::nullopt;
  }

  return AlertFromRow(result.Front());
}

std::vector<Alert> AlertRepository::ListAlerts() const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            SELECT
        )" +
          std::string{kAlertFields} + R"(
            FROM alerts
            ORDER BY created_at DESC, id DESC
        )");

  std::vector<Alert> alerts;
  alerts.reserve(result.Size());
  for (const auto& row : result) {
    alerts.push_back(AlertFromRow(row));
  }
  return alerts;
}

std::vector<Alert> AlertRepository::ListActiveAlerts() const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            SELECT
        )" +
          std::string{kAlertFields} + R"(
            FROM alerts
            WHERE resolved_at IS NULL
            ORDER BY created_at DESC, id DESC
        )");

  std::vector<Alert> alerts;
  alerts.reserve(result.Size());
  for (const auto& row : result) {
    alerts.push_back(AlertFromRow(row));
  }
  return alerts;
}

}  // namespace netwatch::alert_service::alerts
