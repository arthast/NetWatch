#include <alerts/handlers/active_alerts_handler.hpp>

#include <userver/components/component_context.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <alerts/json/alert_json.hpp>
#include <alerts/service/alerts_service_component.hpp>
#include <auth/service/auth_service_component.hpp>
#include <common/auth.hpp>
#include <common/http_response.hpp>

namespace netwatch::api_gateway::alerts {

ActiveAlertsHandler::ActiveAlertsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      alerts_service_(component_context.FindComponent<AlertsServiceComponent>()
                          .GetService()),
      auth_service_(
          component_context.FindComponent<auth::AuthServiceComponent>()
              .GetService()) {}

std::string ActiveAlertsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  try {
    const auto session = common::AuthenticateRequest(request, auth_service_);
    if (!session) {
      return common::ErrorResponse(
          request, userver::server::http::HttpStatus::kUnauthorized,
          "Authorization bearer token is required");
    }

    return common::JsonResponse(
        request,
        SerializeAlerts(alerts_service_.ListActiveAlerts(session->user.id)));
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "alert-service", ex);
  }
}

}  // namespace netwatch::api_gateway::alerts
