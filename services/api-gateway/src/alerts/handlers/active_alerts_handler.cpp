#include <alerts/handlers/active_alerts_handler.hpp>

#include <userver/components/component_context.hpp>
#include <userver/server/http/http_request.hpp>

#include <alerts/json/alert_json.hpp>
#include <alerts/service/alerts_service_component.hpp>
#include <common/http_response.hpp>

namespace netwatch::api_gateway::alerts {

ActiveAlertsHandler::ActiveAlertsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      alerts_service_(component_context.FindComponent<AlertsServiceComponent>()
                          .GetService()) {}

std::string ActiveAlertsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  try {
    return common::JsonResponse(
        request, SerializeAlerts(alerts_service_.ListActiveAlerts()));
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "alert-service", ex);
  }
}

}  // namespace netwatch::api_gateway::alerts
