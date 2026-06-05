#pragma once

#include <string>
#include <string_view>

namespace netwatch::notification_service::notifications {

struct AlertEvent final {
  int version{1};
  std::string event_id;
  std::string event_type;
  std::string producer;
  std::string occurred_at;
  std::string payload;
};

AlertEvent ParseAlertEvent(std::string_view payload);

bool IsSupportedAlertEventType(std::string_view event_type);

}  // namespace netwatch::notification_service::notifications
