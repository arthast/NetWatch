#include <checks/handlers/target_status_handler.hpp>

#include <userver/components/component_context.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <auth/service/auth_service_component.hpp>
#include <checks/json/check_json.hpp>
#include <checks/service/checks_service_component.hpp>
#include <common/auth.hpp>
#include <common/http_response.hpp>
#include <common/path_params.hpp>

namespace netwatch::api_gateway::checks {
namespace {

std::string UpstreamErrorResponse(
    const userver::server::http::HttpRequest& request,
    const UpstreamError& error) {
  const std::string upstream_service{ToString(error.GetService())};
  if (error.IsDeadlineExceeded()) {
    return common::ErrorResponse(
        request, userver::server::http::HttpStatus::kGatewayTimeout,
        upstream_service + " timed out");
  }

  return common::ErrorResponse(request,
                               userver::server::http::HttpStatus::kBadGateway,
                               upstream_service + " is unavailable");
}

}  // namespace

TargetStatusHandler::TargetStatusHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      checks_service_(component_context.FindComponent<ChecksServiceComponent>()
                          .GetService()),
      auth_service_(
          component_context.FindComponent<auth::AuthServiceComponent>()
              .GetService()) {}

std::string TargetStatusHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto target_id = common::ParsePositiveInt64(request.GetPathArg("id"));
  if (!target_id) {
    return common::ErrorResponse(request,
                                 userver::server::http::HttpStatus::kBadRequest,
                                 "target id must be a positive integer");
  }

  try {
    const auto session = common::AuthenticateRequest(request, auth_service_);
    if (!session) {
      return common::ErrorResponse(
          request, userver::server::http::HttpStatus::kUnauthorized,
          "Authorization bearer token is required");
    }

    const auto status =
        checks_service_.GetTargetStatus(session->user.id, *target_id);
    switch (status.kind) {
      case TargetStatusResultKind::kFound:
        return common::JsonResponse(request,
                                    SerializeCheckResult(*status.check));
      case TargetStatusResultKind::kTargetNotFound:
        return common::ErrorResponse(
            request, userver::server::http::HttpStatus::kNotFound,
            "target not found");
      case TargetStatusResultKind::kNoChecks:
        return common::ErrorResponse(
            request, userver::server::http::HttpStatus::kNotFound,
            "target has no checks");
    }

    return common::ErrorResponse(request,
                                 userver::server::http::HttpStatus::kNotFound,
                                 "target has no checks");
  } catch (const UpstreamError& ex) {
    return UpstreamErrorResponse(request, ex);
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "monitor-service", ex);
  }
}
}  // namespace netwatch::api_gateway::checks
