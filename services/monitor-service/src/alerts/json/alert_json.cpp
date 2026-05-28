#include <alerts/json/alert_json.hpp>

#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <common/json.hpp>

namespace monitor_service::alerts {

userver::formats::json::Value SerializeAlert(const Alert& alert) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = alert.id;
  builder["target_id"] = alert.target_id;
  builder["type"] = AlertTypeToString(alert.type);
  builder["severity"] = AlertSeverityToString(alert.severity);
  builder["message"] = alert.message;
  builder["created_at"] = alert.created_at;
  common::SetOptionalField(builder, "resolved_at", alert.resolved_at);
  return builder.ExtractValue();
}

userver::formats::json::Value SerializeAlerts(
    const std::vector<Alert>& alerts) {
  userver::formats::json::ValueBuilder builder(
      userver::formats::common::Type::kArray);
  for (const auto& alert : alerts) {
    userver::formats::json::ValueBuilder item(SerializeAlert(alert));
    builder.PushBack(std::move(item));
  }
  return builder.ExtractValue();
}

}  // namespace monitor_service::alerts
