#include <notifications/repository/notification_repository.hpp>

#include <algorithm>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/storages/postgres/row.hpp>
#include <utility>

namespace netwatch::notification_service::notifications {
namespace {

EmailRecipient RecipientFromRow(
    const userver::storages::postgres::Row& row) {
  return EmailRecipient{
      .id = row["id"].As<std::int64_t>(),
      .email = row["email"].As<std::string>(),
      .is_enabled = row["is_enabled"].As<bool>(),
      .created_at = row["created_at"].As<std::string>(),
      .updated_at = row["updated_at"].As<std::string>(),
  };
}

constexpr std::string_view kRecipientFields = R"(
    id,
    email,
    is_enabled,
    to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS created_at,
    to_char(updated_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS updated_at
)";

}  // namespace

NotificationRepository::NotificationRepository(
    userver::storages::postgres::ClusterPtr pg_cluster)
    : pg_cluster_(std::move(pg_cluster)) {}

NotificationProcessResult NotificationRepository::ProcessAlertEvent(
    const AlertEvent& event) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            WITH incoming AS (
                INSERT INTO notification_events (
                    event_id,
                    event_type,
                    payload,
                    processed_at
                )
                VALUES ($1, $2, $3::jsonb, NOW())
                ON CONFLICT (event_id) DO NOTHING
                RETURNING event_id
            ),
            event_context AS (
                SELECT
                    incoming.event_id,
                    NULLIF($3::jsonb #>> '{target,name}', '') AS target_name,
                    NULLIF($3::jsonb #>> '{target,type}', '') AS target_type
                FROM incoming
            ),
            suppression AS (
                SELECT EXISTS (
                    SELECT 1
                    FROM notification_events AS previous_event
                    WHERE $2 = 'alert.opened'
                      AND event_context.target_name IS NOT NULL
                      AND event_context.target_type IS NOT NULL
                      AND previous_event.event_id <> event_context.event_id
                      AND previous_event.event_type = 'alert.opened'
                      AND NULLIF(previous_event.payload #>> '{target,name}', '') =
                          event_context.target_name
                      AND NULLIF(previous_event.payload #>> '{target,type}', '') =
                          event_context.target_type
                      AND previous_event.received_at >= NOW() - INTERVAL '15 minutes'
                ) AS suppressed
                FROM event_context
            ),
            enabled_recipients AS (
                SELECT id, email
                FROM notification_recipients
                WHERE is_enabled = TRUE
                  AND NOT COALESCE((SELECT suppressed FROM suppression), FALSE)
                ORDER BY id
            ),
            recipient_deliveries AS (
                INSERT INTO notification_deliveries (
                    event_id,
                    recipient_id,
                    recipient_email,
                    channel,
                    status,
                    payload
                )
                SELECT
                    incoming.event_id,
                    enabled_recipients.id,
                    enabled_recipients.email,
                    'email',
                    'pending',
                    $3::jsonb
                FROM incoming
                CROSS JOIN enabled_recipients
                RETURNING id
            ),
            suppressed_delivery AS (
                INSERT INTO notification_deliveries (
                    event_id,
                    channel,
                    status,
                    payload,
                    error_message
                )
                SELECT
                    incoming.event_id,
                    'email',
                    'skipped',
                    $3::jsonb,
                    'suppressed duplicate alert.opened email for target within 15 minutes'
                FROM incoming
                WHERE COALESCE((SELECT suppressed FROM suppression), FALSE)
                RETURNING id
            ),
            skipped_delivery AS (
                INSERT INTO notification_deliveries (
                    event_id,
                    channel,
                    status,
                    payload,
                    error_message
                )
                SELECT
                    incoming.event_id,
                    'email',
                    'skipped',
                    $3::jsonb,
                    'no enabled email recipients'
                FROM incoming
                WHERE NOT COALESCE((SELECT suppressed FROM suppression), FALSE)
                  AND NOT EXISTS (SELECT 1 FROM enabled_recipients)
                RETURNING id
            )
            SELECT
                EXISTS(SELECT 1 FROM incoming) AS inserted,
                COALESCE((SELECT suppressed FROM suppression), FALSE) AS suppressed,
                (SELECT COUNT(*) FROM enabled_recipients) AS recipients_count,
                (
                    (SELECT COUNT(*) FROM recipient_deliveries) +
                    (SELECT COUNT(*) FROM suppressed_delivery) +
                    (SELECT COUNT(*) FROM skipped_delivery)
                ) AS deliveries_count
        )",
      event.event_id, event.event_type, event.payload);

  const auto& row = result.Front();
  return NotificationProcessResult{
      .inserted = row["inserted"].As<bool>(),
      .suppressed = row["suppressed"].As<bool>(),
      .recipients_count = row["recipients_count"].As<std::int64_t>(),
      .deliveries_count = row["deliveries_count"].As<std::int64_t>(),
  };
}

std::vector<PendingNotificationDelivery>
NotificationRepository::AcquirePendingDeliveries(int batch_size) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            WITH acquired AS (
                SELECT id, event_id
                FROM notification_deliveries AS delivery
                WHERE
                    delivery.status = 'pending'
                    OR (
                        delivery.status = 'sending'
                        AND delivery.updated_at < NOW() - INTERVAL '5 minutes'
                    )
                ORDER BY delivery.created_at, delivery.id
                FOR UPDATE SKIP LOCKED
                LIMIT $1
            )
            UPDATE notification_deliveries AS delivery
            SET
                status = 'sending',
                attempts = attempts + 1,
                error_message = NULL,
                updated_at = NOW()
            FROM acquired
            JOIN notification_events AS event
                ON event.event_id = acquired.event_id
            WHERE delivery.id = acquired.id
            RETURNING
                delivery.id,
                delivery.event_id,
                event.event_type,
                delivery.recipient_email,
                delivery.payload::text AS payload
        )",
      batch_size);

  std::vector<PendingNotificationDelivery> deliveries;
  deliveries.reserve(result.Size());
  for (const auto& row : result) {
    deliveries.push_back(PendingNotificationDelivery{
        .id = row["id"].As<std::int64_t>(),
        .event_id = row["event_id"].As<std::string>(),
        .event_type = row["event_type"].As<std::string>(),
        .recipient_email = row["recipient_email"].As<std::string>(),
        .payload = row["payload"].As<std::string>(),
    });
  }
  return deliveries;
}

void NotificationRepository::MarkDeliverySent(std::int64_t delivery_id) const {
  pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                       R"(
            UPDATE notification_deliveries
            SET
                status = 'sent',
                error_message = NULL,
                updated_at = NOW(),
                delivered_at = NOW()
            WHERE id = $1
              AND status = 'sending'
        )",
                       delivery_id);
}

void NotificationRepository::MarkDeliveryFailed(
    std::int64_t delivery_id, std::string_view error_message) const {
  constexpr std::size_t kMaxErrorLength = 1000;
  const auto error = std::string{
      error_message.substr(0, std::min(error_message.size(), kMaxErrorLength))};

  pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                       R"(
            UPDATE notification_deliveries
            SET
                status = 'failed',
                error_message = $2,
                updated_at = NOW()
            WHERE id = $1
              AND status = 'sending'
        )",
                       delivery_id, error);
}

void NotificationRepository::EnsureRecipient(std::string_view email) const {
  pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                       R"(
            INSERT INTO notification_recipients (email, is_enabled)
            VALUES ($1, TRUE)
            ON CONFLICT (email) DO UPDATE
            SET
                is_enabled = TRUE,
                updated_at = NOW()
        )",
                       email);
}

std::vector<EmailRecipient> NotificationRepository::ListRecipients() const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT " + std::string{kRecipientFields} +
          R"(
            FROM notification_recipients
            ORDER BY id
          )");

  std::vector<EmailRecipient> recipients;
  recipients.reserve(result.Size());
  for (const auto& row : result) {
    recipients.push_back(RecipientFromRow(row));
  }
  return recipients;
}

std::optional<EmailRecipient> NotificationRepository::GetRecipientById(
    std::int64_t recipient_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT " + std::string{kRecipientFields} +
          R"(
            FROM notification_recipients
            WHERE id = $1
          )",
      recipient_id);

  if (result.Size() == 0) {
    return std::nullopt;
  }
  return RecipientFromRow(result.Front());
}

std::optional<EmailRecipient> NotificationRepository::GetRecipientByEmail(
    std::string_view email) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT " + std::string{kRecipientFields} +
          R"(
            FROM notification_recipients
            WHERE email = $1
          )",
      email);

  if (result.Size() == 0) {
    return std::nullopt;
  }
  return RecipientFromRow(result.Front());
}

EmailRecipient NotificationRepository::CreateRecipient(
    std::string_view email) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO notification_recipients (email, is_enabled) "
      "VALUES ($1, TRUE) "
      "ON CONFLICT (email) DO UPDATE "
      "SET is_enabled = TRUE, updated_at = NOW() "
      "RETURNING " +
          std::string{kRecipientFields},
      email);

  return RecipientFromRow(result.Front());
}

std::optional<EmailRecipient> NotificationRepository::UpdateRecipient(
    std::int64_t recipient_id, const std::optional<std::string>& email,
    const std::optional<bool>& is_enabled) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "UPDATE notification_recipients "
      "SET email = COALESCE($2, email), "
      "    is_enabled = COALESCE($3, is_enabled), "
      "    updated_at = NOW() "
      "WHERE id = $1 "
      "RETURNING " +
          std::string{kRecipientFields},
      recipient_id, email, is_enabled);

  if (result.Size() == 0) {
    return std::nullopt;
  }
  return RecipientFromRow(result.Front());
}

bool NotificationRepository::DisableRecipient(std::int64_t recipient_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            UPDATE notification_recipients
            SET
                is_enabled = FALSE,
                updated_at = NOW()
            WHERE id = $1
            RETURNING id
        )",
      recipient_id);

  return result.Size() != 0;
}

}  // namespace netwatch::notification_service::notifications
