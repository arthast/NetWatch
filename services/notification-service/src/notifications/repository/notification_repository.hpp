#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/postgres.hpp>
#include <vector>

#include <notifications/events/alert_event.hpp>

namespace netwatch::notification_service::notifications {

struct NotificationProcessResult final {
  bool inserted{false};
  bool suppressed{false};
  std::int64_t recipients_count{0};
  std::int64_t deliveries_count{0};
};

struct PendingNotificationDelivery final {
  std::int64_t id{0};
  std::optional<std::int64_t> user_id;
  std::optional<std::int64_t> target_id;
  std::string event_id;
  std::string event_type;
  std::string recipient_email;
  std::string payload;
};

struct EmailRecipient final {
  std::int64_t id{0};
  std::optional<std::int64_t> user_id;
  std::string email;
  bool is_enabled{false};
  std::string created_at;
  std::string updated_at;
};

struct NotificationDelivery final {
  std::int64_t id{0};
  std::optional<std::int64_t> user_id;
  std::optional<std::int64_t> target_id;
  std::string event_id;
  std::string event_type;
  std::string recipient_email;
  std::string channel;
  std::string status;
  std::int32_t attempts{0};
  std::string error_message;
  std::string next_retry_at;
  std::string created_at;
  std::string updated_at;
  std::string delivered_at;
};

struct ListDeliveriesFilter final {
  int limit{100};
  std::optional<std::int64_t> user_id;
  std::optional<std::int64_t> target_id;
  std::optional<std::string> status;
  std::optional<std::string> event_type;
  std::optional<std::string> recipient_email;
};

struct TargetNotificationSettings final {
  std::int64_t user_id{0};
  std::int64_t target_id{0};
  bool email_enabled{true};
  std::string created_at;
  std::string updated_at;
};

struct TestEmailResult final {
  std::string event_id;
  std::int64_t recipients_count{0};
  std::int64_t deliveries_count{0};
};

class NotificationRepository final {
 public:
  explicit NotificationRepository(
      userver::storages::postgres::ClusterPtr pg_cluster);

  NotificationProcessResult ProcessAlertEvent(const AlertEvent& event) const;
  std::vector<PendingNotificationDelivery> AcquirePendingDeliveries(
      int batch_size) const;
  void MarkDeliverySent(std::int64_t delivery_id) const;
  void MarkDeliveryFailed(std::int64_t delivery_id,
                          std::string_view error_message, int max_attempts,
                          std::chrono::milliseconds retry_delay) const;
  void EnsureRecipient(std::string_view email) const;
  std::vector<EmailRecipient> ListRecipients(
      std::optional<std::int64_t> user_id = std::nullopt) const;
  std::optional<EmailRecipient> GetRecipientById(
      std::int64_t recipient_id) const;
  std::optional<EmailRecipient> GetRecipientByIdForUser(
      std::int64_t recipient_id, std::int64_t user_id) const;
  EmailRecipient CreateRecipient(
      std::string_view email,
      std::optional<std::int64_t> user_id = std::nullopt) const;
  std::optional<EmailRecipient> UpdateRecipient(
      std::int64_t recipient_id, const std::optional<bool>& is_enabled,
      std::optional<std::int64_t> user_id = std::nullopt) const;
  bool DisableRecipient(std::int64_t recipient_id) const;
  bool DisableRecipientForUser(std::int64_t recipient_id,
                               std::int64_t user_id) const;
  std::vector<NotificationDelivery> ListDeliveries(
      const ListDeliveriesFilter& filter) const;
  std::optional<NotificationDelivery> RetryDelivery(
      std::int64_t delivery_id,
      std::optional<std::int64_t> user_id = std::nullopt) const;
  TestEmailResult QueueTestEmail(
      std::optional<std::int64_t> user_id = std::nullopt) const;
  TargetNotificationSettings GetTargetNotificationSettings(
      std::int64_t user_id, std::int64_t target_id) const;
  TargetNotificationSettings UpdateTargetNotificationSettings(
      std::int64_t user_id, std::int64_t target_id, bool email_enabled) const;

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace netwatch::notification_service::notifications
