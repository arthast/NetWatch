#pragma once

#include <string>
#include <string_view>
#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <auth/service/auth_service.hpp>
#include <notifications/service/notifications_service.hpp>

namespace netwatch::api_gateway::notifications {

class NotificationDeliveryRetryHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName =
      "handler-notification-delivery-retry";

  NotificationDeliveryRetryHandler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  const NotificationsService& notifications_service_;
  const auth::AuthService& auth_service_;
};

}  // namespace netwatch::api_gateway::notifications
