#include <targets/handlers/targets_handler.hpp>

#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <auth/service/auth_service_component.hpp>
#include <common/auth.hpp>
#include <common/http_response.hpp>
#include <targets/json/target_json.hpp>
#include <targets/service/targets_service_component.hpp>

namespace netwatch::api_gateway::targets {
using common::ErrorResponse;
using common::JsonResponse;

TargetsHandler::TargetsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      targets_service_(
          component_context.FindComponent<TargetsServiceComponent>()
              .GetService()),
      auth_service_(
          component_context.FindComponent<auth::AuthServiceComponent>()
              .GetService()) {}

std::string TargetsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto method = request.GetMethod();

  if (method == userver::server::http::HttpMethod::kGet) {
    return HandleListTargets(request);
  }

  if (method != userver::server::http::HttpMethod::kPost) {
    return ErrorResponse(request,
                         userver::server::http::HttpStatus::kMethodNotAllowed,
                         "method is not allowed");
  }

  return HandleCreateTarget(request);
}

std::string TargetsHandler::HandleCreateTarget(
    const userver::server::http::HttpRequest& request) const {
  try {
    const auto session = common::AuthenticateRequest(request, auth_service_);
    if (!session) {
      return ErrorResponse(request,
                           userver::server::http::HttpStatus::kUnauthorized,
                           "Authorization bearer token is required");
    }

    const auto request_json =
        userver::formats::json::FromString(request.RequestBody());
    const auto create_request = ParseCreateTargetRequest(request_json);

    const auto target =
        targets_service_.CreateTarget(session->user.id, create_request);
    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    return JsonResponse(request, SerializeTarget(target));
  } catch (const userver::formats::json::Exception& ex) {
    return ErrorResponse(
        request, userver::server::http::HttpStatus::kBadRequest, ex.what());
  } catch (const std::invalid_argument& ex) {
    return ErrorResponse(
        request, userver::server::http::HttpStatus::kBadRequest, ex.what());
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "target-service", ex);
  }
}

std::string TargetsHandler::HandleListTargets(
    const userver::server::http::HttpRequest& request) const {
  try {
    const auto session = common::AuthenticateRequest(request, auth_service_);
    if (!session) {
      return ErrorResponse(request,
                           userver::server::http::HttpStatus::kUnauthorized,
                           "Authorization bearer token is required");
    }

    return JsonResponse(
        request,
        SerializeTargets(targets_service_.ListActiveTargets(session->user.id)));
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "auth-service", ex);
  }
}
}  // namespace netwatch::api_gateway::targets
