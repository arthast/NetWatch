#pragma once

#include <userver/formats/json/value.hpp>
#include <vector>

#include <notification_client/model/email_recipient.hpp>

namespace netwatch::api_gateway::notifications {

netwatch::notification_client::CreateEmailRecipientRequest
ParseCreateEmailRecipientRequest(const userver::formats::json::Value& json);

netwatch::notification_client::UpdateEmailRecipientRequest
ParseUpdateEmailRecipientRequest(const userver::formats::json::Value& json);

userver::formats::json::Value SerializeEmailRecipient(
    const netwatch::notification_client::EmailRecipient& recipient);

userver::formats::json::Value SerializeEmailRecipients(
    const std::vector<netwatch::notification_client::EmailRecipient>&
        recipients);

}  // namespace netwatch::api_gateway::notifications
