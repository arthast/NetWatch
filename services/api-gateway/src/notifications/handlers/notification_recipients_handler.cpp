#include <notifications/handlers/notification_recipients_handler.hpp>

#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <common/http_response.hpp>
#include <notifications/json/recipient_json.hpp>
#include <notifications/service/notifications_service_component.hpp>

namespace netwatch::api_gateway::notifications {
using common::ErrorResponse;
using common::JsonResponse;

NotificationRecipientsHandler::NotificationRecipientsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      notifications_service_(
          component_context.FindComponent<NotificationsServiceComponent>()
              .GetService()) {}

std::string NotificationRecipientsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto method = request.GetMethod();

  if (method == userver::server::http::HttpMethod::kGet) {
    return HandleListRecipients(request);
  }
  if (method == userver::server::http::HttpMethod::kPost) {
    return HandleCreateRecipient(request);
  }

  return ErrorResponse(request,
                       userver::server::http::HttpStatus::kMethodNotAllowed,
                       "method is not allowed");
}

std::string NotificationRecipientsHandler::HandleListRecipients(
    const userver::server::http::HttpRequest& request) const {
  try {
    return JsonResponse(
        request,
        SerializeEmailRecipients(
            notifications_service_.ListEmailRecipients()));
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "notification-service", ex);
  }
}

std::string NotificationRecipientsHandler::HandleCreateRecipient(
    const userver::server::http::HttpRequest& request) const {
  try {
    const auto request_json =
        userver::formats::json::FromString(request.RequestBody());
    const auto create_request =
        ParseCreateEmailRecipientRequest(request_json);

    const auto recipient =
        notifications_service_.CreateEmailRecipient(create_request);
    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    return JsonResponse(request, SerializeEmailRecipient(recipient));
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
