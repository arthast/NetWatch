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

std::vector<EmailRecipient> ToDomainRecipients(
    const proto::ListEmailRecipientsResponse& response) {
  std::vector<EmailRecipient> recipients;
  recipients.reserve(response.recipients_size());
  for (const auto& recipient : response.recipients()) {
    recipients.push_back(ToDomainRecipient(recipient));
  }
  return recipients;
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

}  // namespace netwatch::notification_client
