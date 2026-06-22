#include <auth/handlers/auth_verify_email_handler.hpp>

#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <auth/json/auth_json.hpp>
#include <auth/service/auth_service_component.hpp>
#include <common/http_response.hpp>

namespace netwatch::api_gateway::auth {
namespace {

std::string ParseToken(const userver::formats::json::Value& json) {
  if (!json.IsObject()) {
    throw std::invalid_argument{"request body must be a JSON object"};
  }
  const auto value = json["token"];
  if (value.IsMissing() || value.IsNull()) {
    throw std::invalid_argument{"token is required"};
  }
  return value.As<std::string>();
}

}  // namespace

AuthVerifyEmailHandler::AuthVerifyEmailHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      auth_service_(
          component_context.FindComponent<AuthServiceComponent>()
              .GetService()) {}

std::string AuthVerifyEmailHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
    return common::ErrorResponse(
        request, userver::server::http::HttpStatus::kMethodNotAllowed,
        "method is not allowed");
  }

  try {
    const auto request_json =
        userver::formats::json::FromString(request.RequestBody());
    return common::JsonResponse(
        request,
        SerializeValidatedSession(
            auth_service_.VerifyEmail(ParseToken(request_json))));
  } catch (const userver::formats::json::Exception& ex) {
    return common::ErrorResponse(
        request, userver::server::http::HttpStatus::kBadRequest, ex.what());
  } catch (const std::invalid_argument& ex) {
    return common::ErrorResponse(
        request, userver::server::http::HttpStatus::kBadRequest, ex.what());
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "auth-service", ex);
  }
}

}  // namespace netwatch::api_gateway::auth
