#include <alerts/handlers/alerts_handler.hpp>

#include <userver/components/component_context.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/storages/postgres/component.hpp>

#include <alerts/json/alert_json.hpp>
#include <common/http_response.hpp>

namespace monitor_service::alerts {

AlertsHandler::AlertsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      repository_(
          component_context
              .FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string AlertsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  return common::JsonResponse(request,
                              SerializeAlerts(repository_.ListAlerts()));
}

}  // namespace monitor_service::alerts
