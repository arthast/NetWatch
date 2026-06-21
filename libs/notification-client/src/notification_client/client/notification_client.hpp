#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include <netwatch/notification_service_client.usrv.pb.hpp>
#include <userver/components/component_base.hpp>

#include <notification_client/model/email_recipient.hpp>
#include <notification_client/model/notification_delivery.hpp>

namespace netwatch::notification_client {

class NotificationClient final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "notification-client";

  NotificationClient(const userver::components::ComponentConfig& config,
                     const userver::components::ComponentContext& context);

  std::vector<EmailRecipient> ListEmailRecipients() const;

  std::optional<EmailRecipient> GetEmailRecipient(
      std::int64_t recipient_id) const;

  EmailRecipient CreateEmailRecipient(
      const CreateEmailRecipientRequest& request) const;

  std::optional<EmailRecipient> UpdateEmailRecipient(
      std::int64_t recipient_id,
      const UpdateEmailRecipientRequest& request) const;

  bool DeleteEmailRecipient(std::int64_t recipient_id) const;

  std::vector<EmailRecipient> ListEmailRecipientsForUser(
      std::int64_t user_id) const;

  std::optional<EmailRecipient> GetEmailRecipientForUser(
      std::int64_t recipient_id, std::int64_t user_id) const;

  bool DeleteEmailRecipientForUser(std::int64_t recipient_id,
                                   std::int64_t user_id) const;

  std::vector<NotificationDelivery> ListNotificationDeliveries(
      const ListNotificationDeliveriesRequest& request) const;

  std::optional<NotificationDelivery> RetryNotificationDelivery(
      std::int64_t delivery_id) const;

  std::optional<NotificationDelivery> RetryNotificationDelivery(
      std::int64_t delivery_id, std::int64_t user_id) const;

  SendTestEmailResult SendTestEmail(const SendTestEmailRequest& request) const;

  TargetNotificationSettings GetTargetNotificationSettings(
      std::int64_t user_id, std::int64_t target_id) const;

  TargetNotificationSettings UpdateTargetNotificationSettings(
      std::int64_t user_id, std::int64_t target_id, bool email_enabled) const;

 private:
  netwatch::notification::v1::NotificationServiceClient* grpc_client_;
};

}  // namespace netwatch::notification_client
