#include <notifications/json/test_email_json.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <userver/formats/json/value_builder.hpp>

namespace netwatch::api_gateway::notifications {
namespace notification_client = netwatch::notification_client;
namespace {

template <typename T>
std::optional<T> ReadOptional(const userver::formats::json::Value& json,
                              std::string_view field) {
  const auto value = json[field];
  if (value.IsMissing() || value.IsNull()) {
    return std::nullopt;
  }
  return value.As<T>();
}

}  // namespace

notification_client::SendTestEmailRequest ParseSendTestEmailRequest(
    const userver::formats::json::Value& json) {
  if (!json.IsObject()) {
    throw std::invalid_argument("request body must be a JSON object");
  }

  return notification_client::SendTestEmailRequest{
      .email = ReadOptional<std::string>(json, "email").value_or(""),
  };
}

userver::formats::json::Value SerializeSendTestEmailResult(
    const notification_client::SendTestEmailResult& result) {
  userver::formats::json::ValueBuilder builder;
  builder["event_id"] = result.event_id;
  builder["recipients_count"] = result.recipients_count;
  builder["deliveries_count"] = result.deliveries_count;
  return builder.ExtractValue();
}

}  // namespace netwatch::api_gateway::notifications
