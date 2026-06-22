#include <notifications/json/recipient_json.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace netwatch::api_gateway::notifications {
namespace notification_client = netwatch::notification_client;
namespace {

template <typename T>
std::optional<T> ReadPatchOptional(const userver::formats::json::Value& json,
                                   std::string_view field) {
  const auto value = json[field];
  if (value.IsMissing()) {
    return std::nullopt;
  }
  if (value.IsNull()) {
    throw std::invalid_argument("field '" + std::string{field} +
                                "' must not be null");
  }
  return value.As<T>();
}

}  // namespace

notification_client::UpdateEmailRecipientRequest
ParseUpdateEmailRecipientRequest(const userver::formats::json::Value& json) {
  if (!json.IsObject()) {
    throw std::invalid_argument("request body must be a JSON object");
  }
  if (!json["email"].IsMissing()) {
    throw std::invalid_argument(
        "recipient email cannot be changed; notifications use account email");
  }

  return notification_client::UpdateEmailRecipientRequest{
      .user_id = std::nullopt,
      .is_enabled = ReadPatchOptional<bool>(json, "is_enabled"),
  };
}

userver::formats::json::Value SerializeEmailRecipient(
    const notification_client::EmailRecipient& recipient) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = recipient.id;
  if (recipient.user_id) {
    builder["user_id"] = *recipient.user_id;
  }
  builder["email"] = recipient.email;
  builder["is_enabled"] = recipient.is_enabled;
  builder["created_at"] = recipient.created_at;
  builder["updated_at"] = recipient.updated_at;
  return builder.ExtractValue();
}

userver::formats::json::Value SerializeEmailRecipients(
    const std::vector<notification_client::EmailRecipient>& recipients) {
  userver::formats::json::ValueBuilder builder(
      userver::formats::common::Type::kArray);
  for (const auto& recipient : recipients) {
    userver::formats::json::ValueBuilder item(
        SerializeEmailRecipient(recipient));
    builder.PushBack(std::move(item));
  }
  return builder.ExtractValue();
}

}  // namespace netwatch::api_gateway::notifications
