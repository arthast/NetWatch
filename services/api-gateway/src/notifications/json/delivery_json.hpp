#pragma once

#include <userver/formats/json/value.hpp>
#include <vector>

#include <notification_client/model/notification_delivery.hpp>

namespace netwatch::api_gateway::notifications {

userver::formats::json::Value SerializeNotificationDelivery(
    const netwatch::notification_client::NotificationDelivery& delivery);

userver::formats::json::Value SerializeNotificationDeliveries(
    const std::vector<netwatch::notification_client::NotificationDelivery>&
        deliveries);

}  // namespace netwatch::api_gateway::notifications
