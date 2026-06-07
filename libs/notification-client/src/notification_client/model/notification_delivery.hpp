#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace netwatch::notification_client {

struct NotificationDelivery {
  std::int64_t id{0};
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

struct ListNotificationDeliveriesRequest {
  std::int32_t limit{100};
  std::optional<std::string> status;
  std::optional<std::string> event_type;
  std::optional<std::string> recipient_email;
};

struct SendTestEmailRequest {
  std::string email;
};

struct SendTestEmailResult {
  std::string event_id;
  std::int64_t recipients_count{0};
  std::int64_t deliveries_count{0};
};

}  // namespace netwatch::notification_client
