#pragma once

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
  std::int64_t recipients_count{0};
  std::int64_t deliveries_count{0};
};

struct PendingNotificationDelivery final {
  std::int64_t id{0};
  std::string event_id;
  std::string event_type;
  std::string recipient_email;
  std::string payload;
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
                          std::string_view error_message) const;
  void EnsureRecipient(std::string_view email) const;

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace netwatch::notification_service::notifications
