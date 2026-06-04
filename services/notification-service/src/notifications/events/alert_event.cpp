#include <notifications/events/alert_event.hpp>

#include <stdexcept>
#include <string_view>

#include <userver/formats/json.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value.hpp>

namespace netwatch::notification_service::notifications {
namespace {

std::string ReadRequiredString(const userver::formats::json::Value& json,
                               std::string_view field) {
  auto value = json[std::string{field}].As<std::string>();
  if (value.empty()) {
    throw std::invalid_argument{"alert event field '" + std::string{field} +
                                "' must not be empty"};
  }
  return value;
}

}  // namespace

bool IsSupportedAlertEventType(std::string_view event_type) {
  return event_type == "alert.opened" || event_type == "alert.resolved";
}

AlertEvent ParseAlertEvent(std::string_view payload) {
  const auto json = userver::formats::json::FromString(std::string{payload});

  AlertEvent event{
      .version = json["version"].As<int>(1),
      .event_id = ReadRequiredString(json, "event_id"),
      .event_type = ReadRequiredString(json, "event_type"),
      .producer = ReadRequiredString(json, "producer"),
      .occurred_at = ReadRequiredString(json, "occurred_at"),
      .payload = std::string{payload},
  };

  if (event.version != 1) {
    throw std::invalid_argument{"unsupported alert event version: " +
                                std::to_string(event.version)};
  }
  if (!IsSupportedAlertEventType(event.event_type)) {
    throw std::invalid_argument{"unsupported alert event type: " +
                                event.event_type};
  }

  return event;
}

}  // namespace netwatch::notification_service::notifications
