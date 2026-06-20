#include <notifications/handlers/target_notification_settings_handler.hpp>

#include <stdexcept>

#include <userver/components/component_context.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <auth/service/auth_service_component.hpp>
#include <common/auth.hpp>
#include <common/http_response.hpp>
#include <common/path_params.hpp>
#include <notifications/service/notifications_service_component.hpp>
#include <targets/service/targets_service_component.hpp>

namespace netwatch::api_gateway::notifications {
namespace {

userver::formats::json::Value SerializeSettings(
    const netwatch::notification_client::TargetNotificationSettings& settings) {
  userver::formats::json::ValueBuilder builder;
  builder["user_id"] = settings.user_id;
  builder["target_id"] = settings.target_id;
  builder["email_enabled"] = settings.email_enabled;
  builder["created_at"] = settings.created_at;
  builder["updated_at"] = settings.updated_at;
  return builder.ExtractValue();
}

bool ParseEmailEnabled(const userver::formats::json::Value& json) {
  if (!json.IsObject()) {
    throw std::invalid_argument{"request body must be a JSON object"};
  }

  const auto value = json["email_enabled"];
  if (value.IsMissing() || value.IsNull()) {
    throw std::invalid_argument{"field 'email_enabled' is required"};
  }
  return value.As<bool>();
}

}  // namespace

TargetNotificationSettingsHandler::TargetNotificationSettingsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      notifications_service_(
          component_context.FindComponent<NotificationsServiceComponent>()
              .GetService()),
      targets_service_(
          component_context.FindComponent<targets::TargetsServiceComponent>()
              .GetService()),
      auth_service_(
          component_context.FindComponent<auth::AuthServiceComponent>()
              .GetService()) {}

std::string TargetNotificationSettingsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto target_id = common::ParsePositiveInt64(request.GetPathArg("id"));
  if (!target_id) {
    return common::ErrorResponse(
        request, userver::server::http::HttpStatus::kBadRequest,
        "target id must be a positive integer");
  }

  try {
    const auto session = common::AuthenticateRequest(request, auth_service_);
    if (!session) {
      return common::ErrorResponse(
          request, userver::server::http::HttpStatus::kUnauthorized,
          "Authorization bearer token is required");
    }

    if (!targets_service_.GetTargetById(session->user.id, *target_id)) {
      return common::ErrorResponse(
          request, userver::server::http::HttpStatus::kNotFound,
          "target not found");
    }

    if (request.GetMethod() == userver::server::http::HttpMethod::kGet) {
      return common::JsonResponse(
          request, SerializeSettings(
                       notifications_service_.GetTargetNotificationSettings(
                           session->user.id, *target_id)));
    }

    if (request.GetMethod() != userver::server::http::HttpMethod::kPatch) {
      return common::ErrorResponse(
          request, userver::server::http::HttpStatus::kMethodNotAllowed,
          "method is not allowed");
    }

    const auto request_json =
        userver::formats::json::FromString(request.RequestBody());
    return common::JsonResponse(
        request,
        SerializeSettings(notifications_service_.UpdateTargetNotificationSettings(
            session->user.id, *target_id, ParseEmailEnabled(request_json))));
  } catch (const userver::formats::json::Exception& ex) {
    return common::ErrorResponse(
        request, userver::server::http::HttpStatus::kBadRequest, ex.what());
  } catch (const std::invalid_argument& ex) {
    return common::ErrorResponse(
        request, userver::server::http::HttpStatus::kBadRequest, ex.what());
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "notification-service", ex);
  }
}

}  // namespace netwatch::api_gateway::notifications
