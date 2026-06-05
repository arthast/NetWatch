#include <notifications/grpc/notification_grpc_service.hpp>

#include <algorithm>
#include <cctype>
#include <grpcpp/support/status.h>
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

void ValidateEmail(std::string_view email) {
  if (!IsValidEmail(email)) {
    throw std::invalid_argument{"email must be a valid address"};
  }
}

void FillProtoRecipient(const EmailRecipient& source,
                        proto::EmailRecipient& target) {
  target.set_id(source.id);
  target.set_email(source.email);
  target.set_is_enabled(source.is_enabled);
  target.set_created_at(source.created_at);
  target.set_updated_at(source.updated_at);
}

proto::EmailRecipientResponse MakeRecipientResponse(
    const EmailRecipient& recipient) {
  proto::EmailRecipientResponse response;
  FillProtoRecipient(recipient, *response.mutable_recipient());
  return response;
}

proto::ListEmailRecipientsResponse MakeRecipientsResponse(
    const std::vector<EmailRecipient>& recipients) {
  proto::ListEmailRecipientsResponse response;
  for (const auto& recipient : recipients) {
    FillProtoRecipient(recipient, *response.add_recipients());
  }
  return response;
}

void EnsureEmailCanBeUsed(const NotificationRepository& repository,
                          std::string_view email,
                          std::optional<std::int64_t> current_id) {
  const auto existing = repository.GetRecipientByEmail(email);
  if (existing && (!current_id || existing->id != *current_id)) {
    throw std::invalid_argument{"email recipient already exists"};
  }
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
NotificationGrpcService::ListEmailRecipients(CallContext&,
                                             proto::ListEmailRecipientsRequest&&) {
  return MakeRecipientsResponse(repository_.ListRecipients());
}

NotificationGrpcService::GetEmailRecipientResult
NotificationGrpcService::GetEmailRecipient(CallContext&,
                                           proto::RecipientIdRequest&& request) {
  try {
    ValidateRecipientId(request.id());
    const auto recipient = repository_.GetRecipientById(request.id());
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
    return MakeRecipientResponse(repository_.CreateRecipient(request.email()));
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

NotificationGrpcService::UpdateEmailRecipientResult
NotificationGrpcService::UpdateEmailRecipient(
    CallContext&, proto::UpdateEmailRecipientRequest&& request) {
  try {
    ValidateRecipientId(request.id());
    if (!request.has_email() && !request.has_is_enabled()) {
      throw std::invalid_argument{
          "patch body must contain at least one field"};
    }

    std::optional<std::string> email;
    if (request.has_email()) {
      ValidateEmail(request.email());
      EnsureEmailCanBeUsed(repository_, request.email(), request.id());
      email = request.email();
    }

    const std::optional<bool> is_enabled =
        request.has_is_enabled() ? std::make_optional(request.is_enabled())
                                 : std::nullopt;
    const auto recipient =
        repository_.UpdateRecipient(request.id(), email, is_enabled);
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
    if (!repository_.DisableRecipient(request.id())) {
      return NotFound("email recipient not found");
    }
    return proto::DeleteEmailRecipientResponse{};
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

}  // namespace netwatch::notification_service::notifications
