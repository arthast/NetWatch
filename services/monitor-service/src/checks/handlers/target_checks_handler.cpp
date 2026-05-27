#include <checks/handlers/target_checks_handler.hpp>

#include <userver/components/component_context.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/component.hpp>

#include <checks/json/check_json.hpp>
#include <common/http_response.hpp>
#include <common/path_params.hpp>

namespace monitor_service::checks {

TargetChecksHandler::TargetChecksHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      target_repository_(
          component_context
              .FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()),
      check_repository_(
          component_context
              .FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string TargetChecksHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto target_id = common::ParsePositiveInt64(request.GetPathArg("id"));
  if (!target_id) {
    return common::ErrorResponse(request,
                                 userver::server::http::HttpStatus::kBadRequest,
                                 "target id must be a positive integer");
  }

  if (!target_repository_.GetTargetById(*target_id)) {
    return common::ErrorResponse(request,
                                 userver::server::http::HttpStatus::kNotFound,
                                 "target not found");
  }

  return common::JsonResponse(
      request,
      SerializeCheckResults(check_repository_.ListTargetChecks(*target_id)));
}
}  // namespace monitor_service::checks
