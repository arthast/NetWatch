#include <alerts/handlers/active_alerts_handler.hpp>

#include <userver/components/component_context.hpp>
#include <userver/server/http/http_request.hpp>

#include <alerts/json/alert_json.hpp>
#include <common/http_response.hpp>

namespace netwatch::api_gateway::alerts {

ActiveAlertsHandler::ActiveAlertsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      alert_client_(component_context
                        .FindComponent<netwatch::alert_client::AlertClient>()) {
}

std::string ActiveAlertsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  return common::JsonResponse(
      request, SerializeAlerts(alert_client_.ListActiveAlerts()));
}

}  // namespace netwatch::api_gateway::alerts
