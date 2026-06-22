#include <notifications/json/test_email_json.hpp>

#include <stdexcept>
#include <userver/formats/json/value_builder.hpp>

namespace netwatch::api_gateway::notifications {
namespace notification_client = netwatch::notification_client;

notification_client::SendTestEmailRequest ParseSendTestEmailRequest(
    const userver::formats::json::Value& json) {
  if (!json.IsObject()) {
    throw std::invalid_argument("request body must be a JSON object");
  }
  if (!json["email"].IsMissing()) {
    throw std::invalid_argument(
        "test email recipient cannot be set; notifications use account email");
  }

  return notification_client::SendTestEmailRequest{
      .user_id = std::nullopt,
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
