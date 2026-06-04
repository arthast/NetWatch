#include <notifications/repository/notification_repository.hpp>

#include <algorithm>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/storages/postgres/row.hpp>
#include <utility>

namespace netwatch::notification_service::notifications {

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
            enabled_recipients AS (
                SELECT id, email
                FROM notification_recipients
                WHERE is_enabled = TRUE
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
                WHERE NOT EXISTS (SELECT 1 FROM enabled_recipients)
                RETURNING id
            )
            SELECT
                EXISTS(SELECT 1 FROM incoming) AS inserted,
                (SELECT COUNT(*) FROM enabled_recipients) AS recipients_count,
                (
                    (SELECT COUNT(*) FROM recipient_deliveries) +
                    (SELECT COUNT(*) FROM skipped_delivery)
                ) AS deliveries_count
        )",
      event.event_id, event.event_type, event.payload);

  const auto& row = result.Front();
  return NotificationProcessResult{
      .inserted = row["inserted"].As<bool>(),
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

}  // namespace netwatch::notification_service::notifications
