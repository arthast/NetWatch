#pragma once

#include <userver/formats/json/value.hpp>

#include <notification_client/model/notification_delivery.hpp>

namespace netwatch::api_gateway::notifications {

netwatch::notification_client::SendTestEmailRequest ParseSendTestEmailRequest(
    const userver::formats::json::Value& json);

userver::formats::json::Value SerializeSendTestEmailResult(
    const netwatch::notification_client::SendTestEmailResult& result);

}  // namespace netwatch::api_gateway::notifications
