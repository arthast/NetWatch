#include <notifications/handlers/notification_delivery_retry_handler.hpp>

#include <userver/components/component_context.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <common/http_response.hpp>
#include <common/path_params.hpp>
#include <notifications/json/delivery_json.hpp>
#include <notifications/service/notifications_service_component.hpp>

namespace netwatch::api_gateway::notifications {

NotificationDeliveryRetryHandler::NotificationDeliveryRetryHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      notifications_service_(
          component_context.FindComponent<NotificationsServiceComponent>()
              .GetService()) {}

std::string NotificationDeliveryRetryHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
    return common::ErrorResponse(
        request, userver::server::http::HttpStatus::kMethodNotAllowed,
        "method is not allowed");
  }

  const auto delivery_id = common::ParsePositiveInt64(request.GetPathArg("id"));
  if (!delivery_id) {
    return common::ErrorResponse(
        request, userver::server::http::HttpStatus::kBadRequest,
        "delivery id must be a positive integer");
  }

  try {
    const auto delivery =
        notifications_service_.RetryNotificationDelivery(*delivery_id);
    if (!delivery) {
      return common::ErrorResponse(
          request, userver::server::http::HttpStatus::kNotFound,
          "notification delivery not found or cannot be retried");
    }

    return common::JsonResponse(request,
                                SerializeNotificationDelivery(*delivery));
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "notification-service", ex);
  }
}

}  // namespace netwatch::api_gateway::notifications
