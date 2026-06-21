#include <notifications/repository/notification_repository.hpp>

#include <algorithm>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/storages/postgres/row.hpp>
#include <utility>

namespace netwatch::notification_service::notifications {
namespace {

EmailRecipient RecipientFromRow(const userver::storages::postgres::Row& row) {
  return EmailRecipient{
      .id = row["id"].As<std::int64_t>(),
      .user_id = row["user_id"].As<std::optional<std::int64_t>>(),
      .email = row["email"].As<std::string>(),
      .is_enabled = row["is_enabled"].As<bool>(),
      .created_at = row["created_at"].As<std::string>(),
      .updated_at = row["updated_at"].As<std::string>(),
  };
}

NotificationDelivery DeliveryFromRow(
    const userver::storages::postgres::Row& row) {
  return NotificationDelivery{
      .id = row["id"].As<std::int64_t>(),
      .user_id = row["user_id"].As<std::optional<std::int64_t>>(),
      .target_id = row["target_id"].As<std::optional<std::int64_t>>(),
      .event_id = row["event_id"].As<std::string>(),
      .event_type = row["event_type"].As<std::string>(),
      .recipient_email = row["recipient_email"].As<std::string>(),
      .channel = row["channel"].As<std::string>(),
      .status = row["status"].As<std::string>(),
      .attempts = row["attempts"].As<std::int32_t>(),
      .error_message = row["error_message"].As<std::string>(),
      .next_retry_at = row["next_retry_at"].As<std::string>(),
      .created_at = row["created_at"].As<std::string>(),
      .updated_at = row["updated_at"].As<std::string>(),
      .delivered_at = row["delivered_at"].As<std::string>(),
  };
}

constexpr std::string_view kRecipientFields = R"(
    id,
    user_id,
    email,
    is_enabled,
    to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS created_at,
    to_char(updated_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS updated_at
)";

constexpr std::string_view kDeliveryFields = R"(
    delivery.id,
    delivery.user_id,
    delivery.target_id,
    delivery.event_id,
    event.event_type,
    COALESCE(delivery.recipient_email, '') AS recipient_email,
    delivery.channel,
    delivery.status,
    delivery.attempts,
    COALESCE(delivery.error_message, '') AS error_message,
    COALESCE(to_char(delivery.next_retry_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"'), '') AS next_retry_at,
    to_char(delivery.created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS created_at,
    to_char(delivery.updated_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS updated_at,
    COALESCE(to_char(delivery.delivered_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"'), '') AS delivered_at
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
                    $4::BIGINT AS user_id,
                    $5::BIGINT AS target_id,
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
                      AND ($4::BIGINT IS NULL OR
                           (previous_event.payload #>> '{target,user_id}')::BIGINT = $4::BIGINT)
                      AND ($5::BIGINT IS NULL OR
                           (previous_event.payload #>> '{target,id}')::BIGINT = $5::BIGINT)
                      AND NULLIF(previous_event.payload #>> '{target,name}', '') =
                          event_context.target_name
                      AND NULLIF(previous_event.payload #>> '{target,type}', '') =
                          event_context.target_type
                      AND previous_event.received_at >= NOW() - INTERVAL '15 minutes'
                ) AS suppressed
                FROM event_context
            ),
            target_settings AS (
                SELECT
                    CASE
                        WHEN (SELECT user_id FROM event_context) IS NULL THEN TRUE
                        ELSE COALESCE(
                            (
                                SELECT settings.email_enabled
                                FROM notification_target_settings AS settings
                                WHERE settings.user_id = (SELECT user_id FROM event_context)
                                  AND settings.target_id = (SELECT target_id FROM event_context)
                            ),
                            TRUE
                        )
                    END AS email_enabled
            ),
            enabled_recipients AS (
                SELECT id, email, user_id
                FROM notification_recipients
                WHERE is_enabled = TRUE
                  AND NOT COALESCE((SELECT suppressed FROM suppression), FALSE)
                  AND COALESCE((SELECT email_enabled FROM target_settings), TRUE)
                  AND user_id IS NOT DISTINCT FROM (SELECT user_id FROM event_context)
                ORDER BY id
            ),
            recipient_deliveries AS (
                INSERT INTO notification_deliveries (
                    event_id,
                    user_id,
                    target_id,
                    recipient_id,
                    recipient_email,
                    channel,
                    status,
                    payload
                )
                SELECT
                    incoming.event_id,
                    (SELECT user_id FROM event_context),
                    (SELECT target_id FROM event_context),
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
                    user_id,
                    target_id,
                    channel,
                    status,
                    payload,
                    error_message
                )
                SELECT
                    incoming.event_id,
                    (SELECT user_id FROM event_context),
                    (SELECT target_id FROM event_context),
                    'email',
                    'skipped',
                    $3::jsonb,
                    'suppressed duplicate alert.opened email for target within 15 minutes'
                FROM incoming
                WHERE COALESCE((SELECT suppressed FROM suppression), FALSE)
                RETURNING id
            ),
            disabled_delivery AS (
                INSERT INTO notification_deliveries (
                    event_id,
                    user_id,
                    target_id,
                    channel,
                    status,
                    payload,
                    error_message
                )
                SELECT
                    incoming.event_id,
                    (SELECT user_id FROM event_context),
                    (SELECT target_id FROM event_context),
                    'email',
                    'skipped',
                    $3::jsonb,
                    'email notifications disabled for target'
                FROM incoming
                WHERE NOT COALESCE((SELECT suppressed FROM suppression), FALSE)
                  AND NOT COALESCE((SELECT email_enabled FROM target_settings), TRUE)
                RETURNING id
            ),
            skipped_delivery AS (
                INSERT INTO notification_deliveries (
                    event_id,
                    user_id,
                    target_id,
                    channel,
                    status,
                    payload,
                    error_message
                )
                SELECT
                    incoming.event_id,
                    (SELECT user_id FROM event_context),
                    (SELECT target_id FROM event_context),
                    'email',
                    'skipped',
                    $3::jsonb,
                    'no enabled email recipients'
                FROM incoming
                WHERE NOT COALESCE((SELECT suppressed FROM suppression), FALSE)
                  AND COALESCE((SELECT email_enabled FROM target_settings), TRUE)
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
                    (SELECT COUNT(*) FROM disabled_delivery) +
                    (SELECT COUNT(*) FROM skipped_delivery)
                ) AS deliveries_count
        )",
      event.event_id, event.event_type, event.payload, event.user_id,
      event.target_id);

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
                        delivery.status = 'retry_scheduled'
                        AND delivery.next_retry_at <= NOW()
                    )
                    OR (
                        delivery.status = 'sending'
                        AND delivery.updated_at < NOW() - INTERVAL '5 minutes'
                    )
                ORDER BY COALESCE(delivery.next_retry_at, delivery.created_at), delivery.id
                FOR UPDATE SKIP LOCKED
                LIMIT $1
            )
            UPDATE notification_deliveries AS delivery
            SET
                status = 'sending',
                attempts = attempts + 1,
                error_message = NULL,
                next_retry_at = NULL,
                updated_at = NOW()
            FROM acquired
            JOIN notification_events AS event
                ON event.event_id = acquired.event_id
            WHERE delivery.id = acquired.id
            RETURNING
                delivery.id,
                delivery.user_id,
                delivery.target_id,
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
        .user_id = row["user_id"].As<std::optional<std::int64_t>>(),
        .target_id = row["target_id"].As<std::optional<std::int64_t>>(),
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
                next_retry_at = NULL,
                updated_at = NOW(),
                delivered_at = NOW()
            WHERE id = $1
              AND status = 'sending'
        )",
                       delivery_id);
}

void NotificationRepository::MarkDeliveryFailed(
    std::int64_t delivery_id, std::string_view error_message, int max_attempts,
    std::chrono::milliseconds retry_delay) const {
  constexpr std::size_t kMaxErrorLength = 1000;
  const auto error = std::string{
      error_message.substr(0, std::min(error_message.size(), kMaxErrorLength))};

  pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                       R"(
            UPDATE notification_deliveries
            SET
                status = CASE
                    WHEN attempts < $3 THEN 'retry_scheduled'
                    ELSE 'failed'
                END,
                error_message = $2,
                next_retry_at = CASE
                    WHEN attempts < $3 THEN NOW() + ($4::TEXT || ' milliseconds')::INTERVAL
                    ELSE NULL
                END,
                updated_at = NOW()
            WHERE id = $1
              AND status = 'sending'
        )",
                       delivery_id, error, max_attempts, retry_delay.count());
}

void NotificationRepository::EnsureRecipient(std::string_view email) const {
  pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                       R"(
            INSERT INTO notification_recipients (email, user_id, is_enabled)
            VALUES ($1, NULL, TRUE)
            ON CONFLICT (user_id, email) DO UPDATE
            SET
                is_enabled = TRUE,
                updated_at = NOW()
        )",
                       email);
}

std::vector<EmailRecipient> NotificationRepository::ListRecipients(
    std::optional<std::int64_t> user_id) const {
  const auto result =
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                           "SELECT " + std::string{kRecipientFields} +
                               R"(
            FROM notification_recipients
            WHERE user_id IS NOT DISTINCT FROM $1
            ORDER BY id
          )",
                           user_id);

  std::vector<EmailRecipient> recipients;
  recipients.reserve(result.Size());
  for (const auto& row : result) {
    recipients.push_back(RecipientFromRow(row));
  }
  return recipients;
}

std::optional<EmailRecipient> NotificationRepository::GetRecipientById(
    std::int64_t recipient_id) const {
  const auto result =
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
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

std::optional<EmailRecipient> NotificationRepository::GetRecipientByIdForUser(
    std::int64_t recipient_id, std::int64_t user_id) const {
  const auto result =
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                           "SELECT " + std::string{kRecipientFields} +
                               R"(
            FROM notification_recipients
            WHERE id = $1
              AND user_id = $2
          )",
                           recipient_id, user_id);

  if (result.Size() == 0) {
    return std::nullopt;
  }
  return RecipientFromRow(result.Front());
}

std::optional<EmailRecipient> NotificationRepository::GetRecipientByEmail(
    std::string_view email, std::optional<std::int64_t> user_id) const {
  const auto result =
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                           "SELECT " + std::string{kRecipientFields} +
                               R"(
            FROM notification_recipients
            WHERE email = $1
              AND user_id IS NOT DISTINCT FROM $2
          )",
                           email, user_id);

  if (result.Size() == 0) {
    return std::nullopt;
  }
  return RecipientFromRow(result.Front());
}

EmailRecipient NotificationRepository::CreateRecipient(
    std::string_view email, std::optional<std::int64_t> user_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO notification_recipients (email, user_id, is_enabled) "
      "VALUES ($1, $2, TRUE) "
      "ON CONFLICT (user_id, email) DO UPDATE "
      "SET is_enabled = TRUE, updated_at = NOW() "
      "RETURNING " +
          std::string{kRecipientFields},
      email, user_id);

  return RecipientFromRow(result.Front());
}

std::optional<EmailRecipient> NotificationRepository::UpdateRecipient(
    std::int64_t recipient_id, const std::optional<std::string>& email,
    const std::optional<bool>& is_enabled,
    std::optional<std::int64_t> user_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "UPDATE notification_recipients "
      "SET email = COALESCE($2, email), "
      "    is_enabled = COALESCE($3, is_enabled), "
      "    updated_at = NOW() "
      "WHERE id = $1 "
      "  AND user_id IS NOT DISTINCT FROM $4 "
      "RETURNING " +
          std::string{kRecipientFields},
      recipient_id, email, is_enabled, user_id);

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

bool NotificationRepository::DisableRecipientForUser(
    std::int64_t recipient_id, std::int64_t user_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            UPDATE notification_recipients
            SET
                is_enabled = FALSE,
                updated_at = NOW()
            WHERE id = $1
              AND user_id = $2
            RETURNING id
        )",
      recipient_id, user_id);

  return result.Size() != 0;
}

std::vector<NotificationDelivery> NotificationRepository::ListDeliveries(
    const ListDeliveriesFilter& filter) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT " + std::string{kDeliveryFields} +
          R"(
            FROM notification_deliveries AS delivery
            JOIN notification_events AS event
                ON event.event_id = delivery.event_id
            WHERE ($1::TEXT IS NULL OR delivery.status = $1)
              AND ($2::TEXT IS NULL OR event.event_type = $2)
              AND ($3::TEXT IS NULL OR delivery.recipient_email = $3)
              AND ($5::BIGINT IS NULL OR delivery.user_id = $5)
              AND ($6::BIGINT IS NULL OR delivery.target_id = $6)
            ORDER BY delivery.created_at DESC, delivery.id DESC
            LIMIT $4
          )",
      filter.status, filter.event_type, filter.recipient_email, filter.limit,
      filter.user_id, filter.target_id);

  std::vector<NotificationDelivery> deliveries;
  deliveries.reserve(result.Size());
  for (const auto& row : result) {
    deliveries.push_back(DeliveryFromRow(row));
  }
  return deliveries;
}

std::optional<NotificationDelivery> NotificationRepository::RetryDelivery(
    std::int64_t delivery_id, std::optional<std::int64_t> user_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "WITH updated AS ("
      "    UPDATE notification_deliveries "
      "    SET status = 'pending', "
      "        error_message = NULL, "
      "        next_retry_at = NULL, "
      "        updated_at = NOW() "
      "    WHERE id = $1 "
      "      AND user_id IS NOT DISTINCT FROM $2 "
      "      AND status IN ('failed', 'retry_scheduled') "
      "    RETURNING * "
      ") "
      "SELECT " +
          std::string{kDeliveryFields} +
          " FROM updated AS delivery "
          " JOIN notification_events AS event "
          "   ON event.event_id = delivery.event_id",
      delivery_id, user_id);

  if (result.Size() == 0) {
    return std::nullopt;
  }
  return DeliveryFromRow(result.Front());
}

TestEmailResult NotificationRepository::QueueTestEmail(
    std::string_view email, std::optional<std::int64_t> user_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            WITH event_metadata AS (
                SELECT
                    gen_random_uuid()::TEXT AS event_id,
                    to_char(NOW() AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS occurred_at
            ),
            payload AS (
                SELECT
                    event_metadata.event_id,
                    jsonb_build_object(
                        'version', 1,
                        'event_id', event_metadata.event_id,
                        'event_type', 'alert.opened',
                        'producer', 'notification-service',
                        'occurred_at', event_metadata.occurred_at,
                        'notification_kind', 'test_email',
                        'target', jsonb_build_object(
                            'user_id', $2::bigint,
                            'name', 'Test notification',
                            'type', 'email'
                        ),
                        'alert', jsonb_build_object(
                            'type', 'test_email',
                            'severity', 'info',
                            'message', 'This is a NetWatch test notification email.'
                        )
                    ) AS value
                FROM event_metadata
            ),
            incoming AS (
                INSERT INTO notification_events (
                    event_id,
                    event_type,
                    payload,
                    processed_at
                )
                SELECT
                    payload.event_id,
                    'alert.opened',
                    payload.value,
                    NOW()
                FROM payload
                RETURNING event_id, payload
            ),
            direct_recipient AS (
                SELECT
                    NULL::BIGINT AS id,
                    $2::BIGINT AS user_id,
                    NULLIF($1, '') AS email
                WHERE NULLIF($1, '') IS NOT NULL
            ),
            saved_recipients AS (
                SELECT id, user_id, email
                FROM notification_recipients
                WHERE is_enabled = TRUE
                  AND user_id IS NOT DISTINCT FROM $2::BIGINT
                  AND NULLIF($1, '') IS NULL
                ORDER BY id
            ),
            recipients AS (
                SELECT id, user_id, email FROM direct_recipient
                UNION ALL
                SELECT id, user_id, email FROM saved_recipients
            ),
            recipient_deliveries AS (
                INSERT INTO notification_deliveries (
                    event_id,
                    user_id,
                    recipient_id,
                    recipient_email,
                    channel,
                    status,
                    payload
                )
                SELECT
                    incoming.event_id,
                    recipients.user_id,
                    recipients.id,
                    recipients.email,
                    'email',
                    'pending',
                    incoming.payload
                FROM incoming
                CROSS JOIN recipients
                RETURNING id
            ),
            skipped_delivery AS (
                INSERT INTO notification_deliveries (
                    event_id,
                    user_id,
                    channel,
                    status,
                    payload,
                    error_message
                )
                SELECT
                    incoming.event_id,
                    $2::BIGINT,
                    'email',
                    'skipped',
                    incoming.payload,
                    'no enabled email recipients for test email'
                FROM incoming
                WHERE NOT EXISTS (SELECT 1 FROM recipients)
                RETURNING id
            )
            SELECT
                (SELECT event_id FROM incoming) AS event_id,
                (SELECT COUNT(*) FROM recipients) AS recipients_count,
                (
                    (SELECT COUNT(*) FROM recipient_deliveries) +
                    (SELECT COUNT(*) FROM skipped_delivery)
                ) AS deliveries_count
        )",
      email, user_id);

  const auto& row = result.Front();
  return TestEmailResult{
      .event_id = row["event_id"].As<std::string>(),
      .recipients_count = row["recipients_count"].As<std::int64_t>(),
      .deliveries_count = row["deliveries_count"].As<std::int64_t>(),
  };
}

TargetNotificationSettings
NotificationRepository::GetTargetNotificationSettings(
    std::int64_t user_id, std::int64_t target_id) const {
  const auto result =
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                           R"(
            SELECT
                $1::BIGINT AS user_id,
                $2::BIGINT AS target_id,
                COALESCE(settings.email_enabled, TRUE) AS email_enabled,
                COALESCE(to_char(settings.created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"'), '') AS created_at,
                COALESCE(to_char(settings.updated_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"'), '') AS updated_at
            FROM (SELECT 1) AS base
            LEFT JOIN notification_target_settings AS settings
              ON settings.user_id = $1
             AND settings.target_id = $2
        )",
                           user_id, target_id);

  const auto& row = result.Front();
  return TargetNotificationSettings{
      .user_id = row["user_id"].As<std::int64_t>(),
      .target_id = row["target_id"].As<std::int64_t>(),
      .email_enabled = row["email_enabled"].As<bool>(),
      .created_at = row["created_at"].As<std::string>(),
      .updated_at = row["updated_at"].As<std::string>(),
  };
}

TargetNotificationSettings
NotificationRepository::UpdateTargetNotificationSettings(
    std::int64_t user_id, std::int64_t target_id, bool email_enabled) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            INSERT INTO notification_target_settings (
                user_id,
                target_id,
                email_enabled
            )
            VALUES ($1, $2, $3)
            ON CONFLICT (user_id, target_id) DO UPDATE
            SET
                email_enabled = EXCLUDED.email_enabled,
                updated_at = NOW()
            RETURNING
                user_id,
                target_id,
                email_enabled,
                to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS created_at,
                to_char(updated_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS updated_at
        )",
      user_id, target_id, email_enabled);

  const auto& row = result.Front();
  return TargetNotificationSettings{
      .user_id = row["user_id"].As<std::int64_t>(),
      .target_id = row["target_id"].As<std::int64_t>(),
      .email_enabled = row["email_enabled"].As<bool>(),
      .created_at = row["created_at"].As<std::string>(),
      .updated_at = row["updated_at"].As<std::string>(),
  };
}

}  // namespace netwatch::notification_service::notifications
