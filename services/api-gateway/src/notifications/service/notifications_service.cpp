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
NotificationsService::ListEmailRecipients() const {
  return notification_client_.ListEmailRecipients();
}

std::optional<netwatch::notification_client::EmailRecipient>
NotificationsService::GetEmailRecipient(std::int64_t recipient_id) const {
  return notification_client_.GetEmailRecipient(recipient_id);
}

netwatch::notification_client::EmailRecipient
NotificationsService::CreateEmailRecipient(
    const netwatch::notification_client::CreateEmailRecipientRequest& request)
    const {
  return notification_client_.CreateEmailRecipient(request);
}

std::optional<netwatch::notification_client::EmailRecipient>
NotificationsService::UpdateEmailRecipient(
    std::int64_t recipient_id,
    const netwatch::notification_client::UpdateEmailRecipientRequest& request)
    const {
  if (!HasPatchFields(request)) {
    throw std::invalid_argument{"patch body must contain at least one field"};
  }

  return notification_client_.UpdateEmailRecipient(recipient_id, request);
}

bool NotificationsService::DeleteEmailRecipient(
    std::int64_t recipient_id) const {
  return notification_client_.DeleteEmailRecipient(recipient_id);
}

}  // namespace netwatch::api_gateway::notifications
