#include <auth/handlers/auth_me_handler.hpp>

#include <optional>
#include <string_view>
#include <userver/components/component_context.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <auth/json/auth_json.hpp>
#include <auth/service/auth_service_component.hpp>
#include <common/http_response.hpp>

namespace netwatch::api_gateway::auth {
namespace {

constexpr std::string_view kBearerPrefix = "Bearer ";

std::optional<std::string_view> ExtractBearerToken(
    const userver::server::http::HttpRequest& request) {
  const auto header = request.GetHeader("Authorization");
  if (header.size() <= kBearerPrefix.size() ||
      header.substr(0, kBearerPrefix.size()) != kBearerPrefix) {
    return std::nullopt;
  }
  return header.substr(kBearerPrefix.size());
}

}  // namespace

AuthMeHandler::AuthMeHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      auth_service_(
          component_context.FindComponent<AuthServiceComponent>()
              .GetService()) {}

std::string AuthMeHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  if (request.GetMethod() != userver::server::http::HttpMethod::kGet) {
    return common::ErrorResponse(
        request, userver::server::http::HttpStatus::kMethodNotAllowed,
        "method is not allowed");
  }

  const auto access_token = ExtractBearerToken(request);
  if (!access_token) {
    return common::ErrorResponse(
        request, userver::server::http::HttpStatus::kUnauthorized,
        "Authorization bearer token is required");
  }

  try {
    const auto session = auth_service_.ValidateToken(*access_token);
    if (!session) {
      return common::ErrorResponse(
          request, userver::server::http::HttpStatus::kUnauthorized,
          "access token is invalid");
    }
    return common::JsonResponse(request, SerializeValidatedSession(*session));
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "auth-service", ex);
  }
}

}  // namespace netwatch::api_gateway::auth
