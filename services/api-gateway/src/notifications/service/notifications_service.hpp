#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <notification_client/client/notification_client.hpp>
#include <notification_client/model/email_recipient.hpp>

namespace netwatch::api_gateway::notifications {

class NotificationsService final {
 public:
  explicit NotificationsService(
      const netwatch::notification_client::NotificationClient&
          notification_client);

  std::vector<netwatch::notification_client::EmailRecipient>
  ListEmailRecipients() const;

  std::optional<netwatch::notification_client::EmailRecipient>
  GetEmailRecipient(std::int64_t recipient_id) const;

  netwatch::notification_client::EmailRecipient CreateEmailRecipient(
      const netwatch::notification_client::CreateEmailRecipientRequest& request)
      const;

  std::optional<netwatch::notification_client::EmailRecipient>
  UpdateEmailRecipient(
      std::int64_t recipient_id,
      const netwatch::notification_client::UpdateEmailRecipientRequest& request)
      const;

  bool DeleteEmailRecipient(std::int64_t recipient_id) const;

 private:
  const netwatch::notification_client::NotificationClient&
      notification_client_;
};

}  // namespace netwatch::api_gateway::notifications
