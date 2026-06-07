#include <notifications/json/delivery_json.hpp>

#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace netwatch::api_gateway::notifications {
namespace notification_client = netwatch::notification_client;

userver::formats::json::Value SerializeNotificationDelivery(
    const notification_client::NotificationDelivery& delivery) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = delivery.id;
  builder["event_id"] = delivery.event_id;
  builder["event_type"] = delivery.event_type;
  builder["recipient_email"] = delivery.recipient_email;
  builder["channel"] = delivery.channel;
  builder["status"] = delivery.status;
  builder["attempts"] = delivery.attempts;
  builder["error_message"] = delivery.error_message;
  builder["next_retry_at"] = delivery.next_retry_at;
  builder["created_at"] = delivery.created_at;
  builder["updated_at"] = delivery.updated_at;
  builder["delivered_at"] = delivery.delivered_at;
  return builder.ExtractValue();
}

userver::formats::json::Value SerializeNotificationDeliveries(
    const std::vector<notification_client::NotificationDelivery>& deliveries) {
  userver::formats::json::ValueBuilder builder(
      userver::formats::common::Type::kArray);
  for (const auto& delivery : deliveries) {
    userver::formats::json::ValueBuilder item(
        SerializeNotificationDelivery(delivery));
    builder.PushBack(std::move(item));
  }
  return builder.ExtractValue();
}

}  // namespace netwatch::api_gateway::notifications
