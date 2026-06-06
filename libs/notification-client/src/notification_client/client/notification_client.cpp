#include <notification_client/client/notification_client.hpp>

#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/simple_client_component.hpp>

#include <client_common/call_options.hpp>

namespace netwatch::notification_client {
namespace {

namespace proto = netwatch::notification::v1;

EmailRecipient ToDomainRecipient(const proto::EmailRecipient& recipient) {
  return EmailRecipient{
      .id = recipient.id(),
      .email = recipient.email(),
      .is_enabled = recipient.is_enabled(),
      .created_at = recipient.created_at(),
      .updated_at = recipient.updated_at(),
  };
}

NotificationDelivery ToDomainDelivery(
    const proto::NotificationDelivery& delivery) {
  return NotificationDelivery{
      .id = delivery.id(),
      .event_id = delivery.event_id(),
      .event_type = delivery.event_type(),
      .recipient_email = delivery.recipient_email(),
      .channel = delivery.channel(),
      .status = delivery.status(),
      .attempts = delivery.attempts(),
      .error_message = delivery.error_message(),
      .created_at = delivery.created_at(),
      .updated_at = delivery.updated_at(),
      .delivered_at = delivery.delivered_at(),
  };
}

std::vector<EmailRecipient> ToDomainRecipients(
    const proto::ListEmailRecipientsResponse& response) {
  std::vector<EmailRecipient> recipients;
  recipients.reserve(response.recipients_size());
  for (const auto& recipient : response.recipients()) {
    recipients.push_back(ToDomainRecipient(recipient));
  }
  return recipients;
}

std::vector<NotificationDelivery> ToDomainDeliveries(
    const proto::ListNotificationDeliveriesResponse& response) {
  std::vector<NotificationDelivery> deliveries;
  deliveries.reserve(response.deliveries_size());
  for (const auto& delivery : response.deliveries()) {
    deliveries.push_back(ToDomainDelivery(delivery));
  }
  return deliveries;
}

proto::RecipientIdRequest MakeRecipientIdRequest(std::int64_t recipient_id) {
  proto::RecipientIdRequest request;
  request.set_id(recipient_id);
  return request;
}

std::invalid_argument ToInvalidArgument(
    const userver::ugrpc::client::InvalidArgumentError& ex) {
  return std::invalid_argument{ex.GetStatus().error_message()};
}

SendTestEmailResult ToDomainTestEmailResult(
    const proto::SendTestEmailResponse& response) {
  return SendTestEmailResult{
      .event_id = response.event_id(),
      .recipients_count = response.recipients_count(),
      .deliveries_count = response.deliveries_count(),
  };
}

}  // namespace

NotificationClient::NotificationClient(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      grpc_client_(
          &context
               .FindComponent<userver::ugrpc::client::SimpleClientComponent<
                   proto::NotificationServiceClient>>(
                   "notification-service-client")
               .GetClient()) {}

std::vector<EmailRecipient> NotificationClient::ListEmailRecipients() const {
  return ToDomainRecipients(grpc_client_->ListEmailRecipients(
      proto::ListEmailRecipientsRequest{}, client_common::MakeGrpcCallOptions()));
}

std::optional<EmailRecipient> NotificationClient::GetEmailRecipient(
    std::int64_t recipient_id) const {
  try {
    return ToDomainRecipient(
        grpc_client_
            ->GetEmailRecipient(MakeRecipientIdRequest(recipient_id),
                                client_common::MakeGrpcCallOptions())
            .recipient());
  } catch (const userver::ugrpc::client::NotFoundError&) {
    return std::nullopt;
  }
}

EmailRecipient NotificationClient::CreateEmailRecipient(
    const CreateEmailRecipientRequest& request) const {
  proto::CreateEmailRecipientRequest proto_request;
  proto_request.set_email(request.email);

  try {
    return ToDomainRecipient(
        grpc_client_
            ->CreateEmailRecipient(proto_request,
                                   client_common::MakeGrpcCallOptions())
            .recipient());
  } catch (const userver::ugrpc::client::InvalidArgumentError& ex) {
    throw ToInvalidArgument(ex);
  }
}

std::optional<EmailRecipient> NotificationClient::UpdateEmailRecipient(
    std::int64_t recipient_id,
    const UpdateEmailRecipientRequest& request) const {
  proto::UpdateEmailRecipientRequest proto_request;
  proto_request.set_id(recipient_id);
  if (request.email) {
    proto_request.set_email(*request.email);
  }
  if (request.is_enabled) {
    proto_request.set_is_enabled(*request.is_enabled);
  }

  try {
    return ToDomainRecipient(
        grpc_client_
            ->UpdateEmailRecipient(proto_request,
                                   client_common::MakeGrpcCallOptions())
            .recipient());
  } catch (const userver::ugrpc::client::NotFoundError&) {
    return std::nullopt;
  } catch (const userver::ugrpc::client::InvalidArgumentError& ex) {
    throw ToInvalidArgument(ex);
  }
}

bool NotificationClient::DeleteEmailRecipient(std::int64_t recipient_id) const {
  try {
    grpc_client_->DeleteEmailRecipient(MakeRecipientIdRequest(recipient_id),
                                       client_common::MakeGrpcCallOptions());
    return true;
  } catch (const userver::ugrpc::client::NotFoundError&) {
    return false;
  }
}

std::vector<NotificationDelivery>
NotificationClient::ListNotificationDeliveries(std::int32_t limit) const {
  proto::ListNotificationDeliveriesRequest request;
  request.set_limit(limit);

  return ToDomainDeliveries(grpc_client_->ListNotificationDeliveries(
      request, client_common::MakeGrpcCallOptions()));
}

SendTestEmailResult NotificationClient::SendTestEmail(
    const SendTestEmailRequest& request) const {
  proto::SendTestEmailRequest proto_request;
  if (!request.email.empty()) {
    proto_request.set_email(request.email);
  }

  try {
    return ToDomainTestEmailResult(grpc_client_->SendTestEmail(
        proto_request, client_common::MakeGrpcCallOptions()));
  } catch (const userver::ugrpc::client::InvalidArgumentError& ex) {
    throw ToInvalidArgument(ex);
  }
}

}  // namespace netwatch::notification_client
