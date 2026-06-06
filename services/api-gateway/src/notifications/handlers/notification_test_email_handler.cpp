#include <notifications/handlers/notification_test_email_handler.hpp>

#include <stdexcept>
#include <string>
#include <userver/components/component_context.hpp>
#include <userver/formats/common/type.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <common/http_response.hpp>
#include <notifications/json/test_email_json.hpp>
#include <notifications/service/notifications_service_component.hpp>

namespace netwatch::api_gateway::notifications {
using common::ErrorResponse;
using common::JsonResponse;
namespace {

userver::formats::json::Value ParseOptionalBody(std::string_view body) {
  if (body.empty()) {
    return userver::formats::json::ValueBuilder(
               userver::formats::common::Type::kObject)
        .ExtractValue();
  }
  return userver::formats::json::FromString(std::string{body});
}

}  // namespace

NotificationTestEmailHandler::NotificationTestEmailHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      notifications_service_(
          component_context.FindComponent<NotificationsServiceComponent>()
              .GetService()) {}

std::string NotificationTestEmailHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
    return ErrorResponse(request,
                         userver::server::http::HttpStatus::kMethodNotAllowed,
                         "method is not allowed");
  }

  try {
    const auto request_json = ParseOptionalBody(request.RequestBody());
    const auto test_email_request = ParseSendTestEmailRequest(request_json);
    const auto result =
        notifications_service_.SendTestEmail(test_email_request);

    request.SetResponseStatus(userver::server::http::HttpStatus::kAccepted);
    return JsonResponse(request, SerializeSendTestEmailResult(result));
  } catch (const userver::formats::json::Exception& ex) {
    return ErrorResponse(
        request, userver::server::http::HttpStatus::kBadRequest, ex.what());
  } catch (const std::invalid_argument& ex) {
    return ErrorResponse(
        request, userver::server::http::HttpStatus::kBadRequest, ex.what());
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "notification-service", ex);
  }
}

}  // namespace netwatch::api_gateway::notifications
