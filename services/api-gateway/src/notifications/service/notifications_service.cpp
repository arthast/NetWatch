#include <notifications/service/notifications_service.hpp>

#include <stdexcept>

namespace netwatch::api_gateway::notifications {
namespace {

bool HasPatchFields(
    const netwatch::notification_client::UpdateEmailRecipientRequest& request) {
  return request.email || request.is_enabled;
}

}  // namespace

NotificationsService::NotificationsService(
    const netwatch::notification_client::NotificationClient&
        notification_client)
    : notification_client_(notification_client) {}

std::vector<netwatch::notification_client::EmailRecipient>
NotificationsService::ListEmailRecipients(std::int64_t user_id) const {
  return notification_client_.ListEmailRecipientsForUser(user_id);
}

std::optional<netwatch::notification_client::EmailRecipient>
NotificationsService::GetEmailRecipient(std::int64_t user_id,
                                        std::int64_t recipient_id) const {
  return notification_client_.GetEmailRecipientForUser(recipient_id, user_id);
}

netwatch::notification_client::EmailRecipient
NotificationsService::CreateEmailRecipient(
    std::int64_t user_id,
    const netwatch::notification_client::CreateEmailRecipientRequest& request)
    const {
  auto scoped_request = request;
  scoped_request.user_id = user_id;
  return notification_client_.CreateEmailRecipient(scoped_request);
}

std::optional<netwatch::notification_client::EmailRecipient>
NotificationsService::UpdateEmailRecipient(
    std::int64_t user_id, std::int64_t recipient_id,
    const netwatch::notification_client::UpdateEmailRecipientRequest& request)
    const {
  if (!HasPatchFields(request)) {
    throw std::invalid_argument{"patch body must contain at least one field"};
  }

  auto scoped_request = request;
  scoped_request.user_id = user_id;
  return notification_client_.UpdateEmailRecipient(recipient_id,
                                                   scoped_request);
}

bool NotificationsService::DeleteEmailRecipient(
    std::int64_t user_id, std::int64_t recipient_id) const {
  return notification_client_.DeleteEmailRecipientForUser(recipient_id,
                                                          user_id);
}

std::vector<netwatch::notification_client::NotificationDelivery>
NotificationsService::ListNotificationDeliveries(
    std::int64_t user_id,
    const netwatch::notification_client::ListNotificationDeliveriesRequest&
        request) const {
  auto scoped_request = request;
  scoped_request.user_id = user_id;
  return notification_client_.ListNotificationDeliveries(scoped_request);
}

std::optional<netwatch::notification_client::NotificationDelivery>
NotificationsService::RetryNotificationDelivery(
    std::int64_t user_id, std::int64_t delivery_id) const {
  return notification_client_.RetryNotificationDelivery(delivery_id, user_id);
}

netwatch::notification_client::SendTestEmailResult
NotificationsService::SendTestEmail(
    std::int64_t user_id,
    const netwatch::notification_client::SendTestEmailRequest& request) const {
  auto scoped_request = request;
  scoped_request.user_id = user_id;
  return notification_client_.SendTestEmail(scoped_request);
}

netwatch::notification_client::TargetNotificationSettings
NotificationsService::GetTargetNotificationSettings(
    std::int64_t user_id, std::int64_t target_id) const {
  return notification_client_.GetTargetNotificationSettings(user_id, target_id);
}

netwatch::notification_client::TargetNotificationSettings
NotificationsService::UpdateTargetNotificationSettings(
    std::int64_t user_id, std::int64_t target_id, bool email_enabled) const {
  return notification_client_.UpdateTargetNotificationSettings(
      user_id, target_id, email_enabled);
}

}  // namespace netwatch::api_gateway::notifications
