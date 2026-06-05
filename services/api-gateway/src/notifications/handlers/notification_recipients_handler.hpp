#pragma once

#include <string>
#include <string_view>
#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <notifications/service/notifications_service.hpp>

namespace netwatch::api_gateway::notifications {

class NotificationRecipientsHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName =
      "handler-notification-recipients";

  NotificationRecipientsHandler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  std::string HandleListRecipients(
      const userver::server::http::HttpRequest& request) const;

  std::string HandleCreateRecipient(
      const userver::server::http::HttpRequest& request) const;

  const NotificationsService& notifications_service_;
};

}  // namespace netwatch::api_gateway::notifications
