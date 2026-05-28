#include <checks/handlers/target_status_handler.hpp>

#include <userver/components/component_context.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <checks/json/check_json.hpp>
#include <common/http_response.hpp>
#include <common/path_params.hpp>

namespace monitor_service::checks {

TargetStatusHandler::TargetStatusHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      check_client_(component_context.FindComponent<CheckClient>()),
      target_client_(component_context.FindComponent<target::TargetClient>()) {}

std::string TargetStatusHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto target_id = common::ParsePositiveInt64(request.GetPathArg("id"));
  if (!target_id) {
    return common::ErrorResponse(request,
                                 userver::server::http::HttpStatus::kBadRequest,
                                 "target id must be a positive integer");
  }

  const auto status = check_client_.GetTargetStatus(*target_id);
  if (!status) {
    if (!target_client_.GetTargetById(*target_id)) {
      return common::ErrorResponse(request,
                                   userver::server::http::HttpStatus::kNotFound,
                                   "target not found");
    }

    return common::ErrorResponse(request,
                                 userver::server::http::HttpStatus::kNotFound,
                                 "target has no checks");
  }

  return common::JsonResponse(request, SerializeCheckResult(*status));
}
}  // namespace monitor_service::checks
