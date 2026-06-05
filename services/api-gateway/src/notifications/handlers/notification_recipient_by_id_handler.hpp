#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <notifications/service/notifications_service.hpp>

namespace netwatch::api_gateway::notifications {

class NotificationRecipientByIdHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName =
      "handler-notification-recipient-by-id";

  NotificationRecipientByIdHandler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  std::string HandleGetRecipient(
      const userver::server::http::HttpRequest& request,
      std::int64_t recipient_id) const;

  std::string HandlePatchRecipient(
      const userver::server::http::HttpRequest& request,
      std::int64_t recipient_id) const;

  std::string HandleDeleteRecipient(
      const userver::server::http::HttpRequest& request,
      std::int64_t recipient_id) const;

  const NotificationsService& notifications_service_;
};

}  // namespace netwatch::api_gateway::notifications
