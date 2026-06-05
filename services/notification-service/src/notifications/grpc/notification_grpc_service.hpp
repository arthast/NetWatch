#pragma once

#include <string_view>

#include <netwatch/notification_service_service.usrv.pb.hpp>
#include <userver/components/component.hpp>

#include <notifications/repository/notification_repository.hpp>

namespace netwatch::notification_service::notifications {

class NotificationGrpcService final
    : public netwatch::notification::v1::NotificationServiceBase::Component {
 public:
  static constexpr std::string_view kName = "notification-grpc-service";

  NotificationGrpcService(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& context);

  ListEmailRecipientsResult ListEmailRecipients(
      CallContext& context,
      netwatch::notification::v1::ListEmailRecipientsRequest&& request)
      override;

  GetEmailRecipientResult GetEmailRecipient(
      CallContext& context,
      netwatch::notification::v1::RecipientIdRequest&& request) override;

  CreateEmailRecipientResult CreateEmailRecipient(
      CallContext& context,
      netwatch::notification::v1::CreateEmailRecipientRequest&& request)
      override;

  UpdateEmailRecipientResult UpdateEmailRecipient(
      CallContext& context,
      netwatch::notification::v1::UpdateEmailRecipientRequest&& request)
      override;

  DeleteEmailRecipientResult DeleteEmailRecipient(
      CallContext& context,
      netwatch::notification::v1::RecipientIdRequest&& request) override;

 private:
  NotificationRepository repository_;
};

}  // namespace netwatch::notification_service::notifications
