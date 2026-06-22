#include <notifications/grpc/notification_grpc_service.hpp>

#include <grpcpp/support/status.h>
#include <algorithm>
#include <cctype>
#include <optional>
#include <stdexcept>
#include <string>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/component.hpp>
#include <utility>
#include <vector>

namespace netwatch::notification_service::notifications {
namespace {

namespace proto = netwatch::notification::v1;

grpc::Status InvalidArgument(std::string message) {
  return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, std::move(message)};
}

grpc::Status NotFound(std::string message) {
  return grpc::Status{grpc::StatusCode::NOT_FOUND, std::move(message)};
}

bool IsValidEmail(std::string_view email) {
  if (email.size() < 3 || email.size() > 320) {
    return false;
  }

  const auto at = email.find('@');
  if (at == std::string_view::npos || at == 0 || at + 1 >= email.size()) {
    return false;
  }
  if (email.find('@', at + 1) != std::string_view::npos) {
    return false;
  }
  if (email.find('.', at + 1) == std::string_view::npos) {
    return false;
  }

  return std::none_of(email.begin(), email.end(), [](unsigned char ch) {
    return std::isspace(ch) != 0 || std::iscntrl(ch) != 0;
  });
}

void ValidateRecipientId(std::int64_t recipient_id) {
  if (recipient_id <= 0) {
    throw std::invalid_argument{"recipient id must be a positive integer"};
  }
}

void ValidateDeliveryId(std::int64_t delivery_id) {
  if (delivery_id <= 0) {
    throw std::invalid_argument{"delivery id must be a positive integer"};
  }
}

void ValidateUserId(std::int64_t user_id) {
  if (user_id <= 0) {
    throw std::invalid_argument{"user id must be a positive integer"};
  }
}

void ValidateTargetId(std::int64_t target_id) {
  if (target_id <= 0) {
    throw std::invalid_argument{"target id must be a positive integer"};
  }
}

void ValidateEmail(std::string_view email) {
  if (!IsValidEmail(email)) {
    throw std::invalid_argument{"email must be a valid address"};
  }
}

void FillProtoRecipient(const EmailRecipient& source,
                        proto::EmailRecipient& target) {
  target.set_id(source.id);
  if (source.user_id) {
    target.set_user_id(*source.user_id);
  }
  target.set_email(source.email);
  target.set_is_enabled(source.is_enabled);
  target.set_created_at(source.created_at);
  target.set_updated_at(source.updated_at);
}

void FillProtoDelivery(const NotificationDelivery& source,
                       proto::NotificationDelivery& target) {
  target.set_id(source.id);
  if (source.user_id) {
    target.set_user_id(*source.user_id);
  }
  if (source.target_id) {
    target.set_target_id(*source.target_id);
  }
  target.set_event_id(source.event_id);
  target.set_event_type(source.event_type);
  target.set_recipient_email(source.recipient_email);
  target.set_channel(source.channel);
  target.set_status(source.status);
  target.set_attempts(source.attempts);
  target.set_error_message(source.error_message);
  target.set_next_retry_at(source.next_retry_at);
  target.set_created_at(source.created_at);
  target.set_updated_at(source.updated_at);
  target.set_delivered_at(source.delivered_at);
}

proto::EmailRecipientResponse MakeRecipientResponse(
    const EmailRecipient& recipient) {
  proto::EmailRecipientResponse response;
  FillProtoRecipient(recipient, *response.mutable_recipient());
  return response;
}

proto::ListNotificationDeliveriesResponse MakeDeliveriesResponse(
    const std::vector<NotificationDelivery>& deliveries) {
  proto::ListNotificationDeliveriesResponse response;
  for (const auto& delivery : deliveries) {
    FillProtoDelivery(delivery, *response.add_deliveries());
  }
  return response;
}

proto::NotificationDeliveryResponse MakeDeliveryResponse(
    const NotificationDelivery& delivery) {
  proto::NotificationDeliveryResponse response;
  FillProtoDelivery(delivery, *response.mutable_delivery());
  return response;
}

proto::SendTestEmailResponse MakeTestEmailResponse(
    const TestEmailResult& result) {
  proto::SendTestEmailResponse response;
  response.set_event_id(result.event_id);
  response.set_recipients_count(result.recipients_count);
  response.set_deliveries_count(result.deliveries_count);
  return response;
}

proto::TargetNotificationSettingsResponse MakeSettingsResponse(
    const TargetNotificationSettings& settings) {
  proto::TargetNotificationSettingsResponse response;
  auto& target = *response.mutable_settings();
  target.set_user_id(settings.user_id);
  target.set_target_id(settings.target_id);
  target.set_email_enabled(settings.email_enabled);
  target.set_created_at(settings.created_at);
  target.set_updated_at(settings.updated_at);
  return response;
}

int NormalizeDeliveriesLimit(int limit) {
  constexpr int kDefaultLimit = 100;
  constexpr int kMaxLimit = 500;

  if (limit == 0) {
    return kDefaultLimit;
  }
  if (limit < 0 || limit > kMaxLimit) {
    throw std::invalid_argument{"limit must be between 1 and 500"};
  }
  return limit;
}

bool IsValidDeliveryStatus(std::string_view status) {
  return status == "pending" || status == "sending" ||
         status == "retry_scheduled" || status == "sent" ||
         status == "skipped" || status == "failed";
}

ListDeliveriesFilter MakeDeliveriesFilter(
    const proto::ListNotificationDeliveriesRequest& request) {
  ListDeliveriesFilter filter{
      .limit = NormalizeDeliveriesLimit(request.limit()),
      .user_id = request.has_user_id() ? std::make_optional(request.user_id())
                                       : std::nullopt,
      .target_id = request.has_target_id()
                       ? std::make_optional(request.target_id())
                       : std::nullopt,
      .status = std::nullopt,
      .event_type = std::nullopt,
      .recipient_email = std::nullopt,
  };

  if (request.has_status()) {
    if (!IsValidDeliveryStatus(request.status())) {
      throw std::invalid_argument{
          "status must be one of pending, sending, retry_scheduled, sent, "
          "skipped, failed"};
    }
    filter.status = request.status();
  }

  if (request.has_event_type()) {
    if (!IsSupportedAlertEventType(request.event_type())) {
      throw std::invalid_argument{
          "event_type must be alert.opened or alert.resolved"};
    }
    filter.event_type = request.event_type();
  }

  if (request.has_recipient_email()) {
    ValidateEmail(request.recipient_email());
    filter.recipient_email = request.recipient_email();
  }

  if (filter.user_id) {
    ValidateUserId(*filter.user_id);
  }
  if (filter.target_id) {
    ValidateTargetId(*filter.target_id);
  }

  return filter;
}

proto::ListEmailRecipientsResponse MakeRecipientsResponse(
    const std::vector<EmailRecipient>& recipients) {
  proto::ListEmailRecipientsResponse response;
  for (const auto& recipient : recipients) {
    FillProtoRecipient(recipient, *response.add_recipients());
  }
  return response;
}

}  // namespace

NotificationGrpcService::NotificationGrpcService(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : proto::NotificationServiceBase::Component(config, context),
      repository_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

NotificationGrpcService::ListEmailRecipientsResult
NotificationGrpcService::ListEmailRecipients(
    CallContext&, proto::ListEmailRecipientsRequest&& request) {
  try {
    if (request.has_user_id()) {
      ValidateUserId(request.user_id());
      return MakeRecipientsResponse(
          repository_.ListRecipients(request.user_id()));
    }
    return MakeRecipientsResponse(repository_.ListRecipients());
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

NotificationGrpcService::GetEmailRecipientResult
NotificationGrpcService::GetEmailRecipient(
    CallContext&, proto::RecipientIdRequest&& request) {
  try {
    ValidateRecipientId(request.id());
    std::optional<EmailRecipient> recipient;
    if (request.has_user_id()) {
      ValidateUserId(request.user_id());
      recipient =
          repository_.GetRecipientByIdForUser(request.id(), request.user_id());
    } else {
      recipient = repository_.GetRecipientById(request.id());
    }
    if (!recipient) {
      return NotFound("email recipient not found");
    }
    return MakeRecipientResponse(*recipient);
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

NotificationGrpcService::CreateEmailRecipientResult
NotificationGrpcService::CreateEmailRecipient(
    CallContext&, proto::CreateEmailRecipientRequest&& request) {
  try {
    ValidateEmail(request.email());
    std::optional<std::int64_t> user_id;
    if (request.has_user_id()) {
      ValidateUserId(request.user_id());
      user_id = request.user_id();
    }
    return MakeRecipientResponse(
        repository_.CreateRecipient(request.email(), user_id));
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

NotificationGrpcService::UpdateEmailRecipientResult
NotificationGrpcService::UpdateEmailRecipient(
    CallContext&, proto::UpdateEmailRecipientRequest&& request) {
  try {
    ValidateRecipientId(request.id());
    std::optional<std::int64_t> user_id;
    if (request.has_user_id()) {
      ValidateUserId(request.user_id());
      user_id = request.user_id();
    }
    if (!request.has_is_enabled()) {
      throw std::invalid_argument{"patch body must contain at least one field"};
    }

    const std::optional<bool> is_enabled =
        request.has_is_enabled() ? std::make_optional(request.is_enabled())
                                 : std::nullopt;
    const auto recipient =
        repository_.UpdateRecipient(request.id(), is_enabled, user_id);
    if (!recipient) {
      return NotFound("email recipient not found");
    }
    return MakeRecipientResponse(*recipient);
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

NotificationGrpcService::DeleteEmailRecipientResult
NotificationGrpcService::DeleteEmailRecipient(
    CallContext&, proto::RecipientIdRequest&& request) {
  try {
    ValidateRecipientId(request.id());
    bool deleted = false;
    if (request.has_user_id()) {
      ValidateUserId(request.user_id());
      deleted =
          repository_.DisableRecipientForUser(request.id(), request.user_id());
    } else {
      deleted = repository_.DisableRecipient(request.id());
    }
    if (!deleted) {
      return NotFound("email recipient not found");
    }
    return proto::DeleteEmailRecipientResponse{};
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

NotificationGrpcService::ListNotificationDeliveriesResult
NotificationGrpcService::ListNotificationDeliveries(
    CallContext&, proto::ListNotificationDeliveriesRequest&& request) {
  try {
    return MakeDeliveriesResponse(
        repository_.ListDeliveries(MakeDeliveriesFilter(request)));
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

NotificationGrpcService::RetryNotificationDeliveryResult
NotificationGrpcService::RetryNotificationDelivery(
    CallContext&, proto::DeliveryIdRequest&& request) {
  try {
    ValidateDeliveryId(request.id());
    std::optional<std::int64_t> user_id;
    if (request.has_user_id()) {
      ValidateUserId(request.user_id());
      user_id = request.user_id();
    }
    const auto delivery = repository_.RetryDelivery(request.id(), user_id);
    if (!delivery) {
      return NotFound("notification delivery not found or cannot be retried");
    }
    return MakeDeliveryResponse(*delivery);
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

NotificationGrpcService::SendTestEmailResult
NotificationGrpcService::SendTestEmail(CallContext&,
                                       proto::SendTestEmailRequest&& request) {
  try {
    std::optional<std::int64_t> user_id;
    if (request.has_user_id()) {
      ValidateUserId(request.user_id());
      user_id = request.user_id();
    }
    return MakeTestEmailResponse(repository_.QueueTestEmail(user_id));
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

NotificationGrpcService::GetTargetNotificationSettingsResult
NotificationGrpcService::GetTargetNotificationSettings(
    CallContext&, proto::GetTargetNotificationSettingsRequest&& request) {
  try {
    ValidateUserId(request.user_id());
    ValidateTargetId(request.target_id());
    return MakeSettingsResponse(repository_.GetTargetNotificationSettings(
        request.user_id(), request.target_id()));
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

NotificationGrpcService::UpdateTargetNotificationSettingsResult
NotificationGrpcService::UpdateTargetNotificationSettings(
    CallContext&, proto::UpdateTargetNotificationSettingsRequest&& request) {
  try {
    ValidateUserId(request.user_id());
    ValidateTargetId(request.target_id());
    return MakeSettingsResponse(repository_.UpdateTargetNotificationSettings(
        request.user_id(), request.target_id(), request.email_enabled()));
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

}  // namespace netwatch::notification_service::notifications
