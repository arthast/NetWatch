#include <alerts/repository/alert_outbox_repository.hpp>

#include <optional>
#include <stdexcept>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/storages/postgres/row.hpp>
#include <utility>

namespace netwatch::alert_service::alerts {
namespace {

AlertOutboxEvent AlertOutboxEventFromRow(
    const userver::storages::postgres::Row& row) {
  return AlertOutboxEvent{
      .event_id = row["event_id"].As<std::string>(),
      .event_type =
          AlertEventTypeFromString(row["event_type"].As<std::string>()),
      .aggregate_type = row["aggregate_type"].As<std::string>(),
      .aggregate_id = row["aggregate_id"].As<std::int64_t>(),
      .partition_key = row["partition_key"].As<std::string>(),
      .payload = row["payload"].As<std::string>(),
      .status = AlertOutboxStatusFromString(row["status"].As<std::string>()),
      .attempts = row["attempts"].As<int>(),
      .next_retry_at = row["next_retry_at"].As<std::string>(),
      .last_error = row["last_error"].As<std::optional<std::string>>(),
      .created_at = row["created_at"].As<std::string>(),
      .published_at = row["published_at"].As<std::optional<std::string>>(),
  };
}

constexpr std::string_view kOutboxFields = R"(
    event_id,
    event_type,
    aggregate_type,
    aggregate_id,
    partition_key,
    payload::text AS payload,
    status,
    attempts,
    to_char(next_retry_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS next_retry_at,
    last_error,
    to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS created_at,
    to_char(published_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS published_at
)";

}  // namespace

AlertOutboxRepository::AlertOutboxRepository(
    userver::storages::postgres::ClusterPtr pg_cluster)
    : pg_cluster_(std::move(pg_cluster)) {}

std::vector<AlertOutboxEvent> AlertOutboxRepository::AcquirePendingEvents(
    int limit) const {
  if (limit <= 0) {
    throw std::invalid_argument{"alert outbox acquire limit must be positive"};
  }

  const auto result =
      pg_cluster_->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          R"(
            WITH next_events AS (
                SELECT event_id
                FROM alert_outbox_events
                WHERE status IN ('pending', 'failed')
                  AND next_retry_at <= NOW()
                ORDER BY created_at ASC
                LIMIT $1
                FOR UPDATE SKIP LOCKED
            )
            UPDATE alert_outbox_events AS outbox
            SET status = 'publishing',
                attempts = outbox.attempts + 1,
                last_error = NULL
            FROM next_events
            WHERE outbox.event_id = next_events.event_id
            RETURNING
        )" + std::string{kOutboxFields},
          limit);

  std::vector<AlertOutboxEvent> events;
  events.reserve(result.Size());
  for (const auto& row : result) {
    events.push_back(AlertOutboxEventFromRow(row));
  }

  return events;
}

void AlertOutboxRepository::MarkPublished(const std::string& event_id) const {
  pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                       R"(
            UPDATE alert_outbox_events
            SET status = 'published',
                published_at = NOW(),
                last_error = NULL
            WHERE event_id = $1
        )",
                       event_id);
}

void AlertOutboxRepository::MarkFailed(
    const std::string& event_id, const std::string& last_error,
    std::chrono::milliseconds retry_delay) const {
  pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                       R"(
            UPDATE alert_outbox_events
            SET status = 'failed',
                next_retry_at = NOW() + ($3::bigint * INTERVAL '1 millisecond'),
                last_error = $2
            WHERE event_id = $1
        )",
                       event_id, last_error, retry_delay.count());
}

}  // namespace netwatch::alert_service::alerts
