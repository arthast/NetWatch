#include <alerts/handlers/alerts_handler.hpp>

#include <userver/components/component_context.hpp>
#include <userver/server/http/http_request.hpp>

#include <alerts/json/alert_json.hpp>
#include <common/http_response.hpp>

namespace netwatch::api_gateway::alerts {

AlertsHandler::AlertsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      alert_client_(component_context
                        .FindComponent<netwatch::alert_client::AlertClient>()) {
}

std::string AlertsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  try {
    return common::JsonResponse(request,
                                SerializeAlerts(alert_client_.ListAlerts()));
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "alert-service", ex);
  }
}

}  // namespace netwatch::api_gateway::alerts
