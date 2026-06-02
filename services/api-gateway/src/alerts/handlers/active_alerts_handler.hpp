#pragma once

#include <string>
#include <string_view>
#include <userver/server/handlers/http_handler_base.hpp>

#include <alerts/service/alerts_service.hpp>

namespace netwatch::api_gateway::alerts {

class ActiveAlertsHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-active-alerts";

  ActiveAlertsHandler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  const AlertsService& alerts_service_;
};

}  // namespace netwatch::api_gateway::alerts
