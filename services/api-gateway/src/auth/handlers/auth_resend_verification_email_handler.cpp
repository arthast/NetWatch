#include <auth/handlers/auth_resend_verification_email_handler.hpp>

#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <auth/json/auth_json.hpp>
#include <auth/service/auth_service_component.hpp>
#include <common/auth.hpp>
#include <common/http_response.hpp>

namespace netwatch::api_gateway::auth {

AuthResendVerificationEmailHandler::AuthResendVerificationEmailHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      auth_service_(
          component_context.FindComponent<AuthServiceComponent>()
              .GetService()) {}

std::string AuthResendVerificationEmailHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
    return common::ErrorResponse(
        request, userver::server::http::HttpStatus::kMethodNotAllowed,
        "method is not allowed");
  }

  try {
    const auto session = common::AuthenticateRequest(request, auth_service_);
    if (!session) {
      return common::ErrorResponse(
          request, userver::server::http::HttpStatus::kUnauthorized,
          "Authorization bearer token is required");
    }

    return common::JsonResponse(
        request,
        SerializeValidatedSession(
            auth_service_.ResendVerificationEmail(session->user.id)));
  } catch (const std::invalid_argument& ex) {
    return common::ErrorResponse(
        request, userver::server::http::HttpStatus::kBadRequest, ex.what());
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "auth-service", ex);
  }
}

}  // namespace netwatch::api_gateway::auth
