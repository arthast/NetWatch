#include <notifications/handlers/notification_deliveries_handler.hpp>

#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <userver/components/component_context.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <common/http_response.hpp>
#include <notifications/json/delivery_json.hpp>
#include <notifications/service/notifications_service_component.hpp>

namespace netwatch::api_gateway::notifications {
using common::ErrorResponse;
using common::JsonResponse;
namespace {

std::optional<std::int32_t> ParseLimit(std::string_view value) {
  if (value.empty()) {
    return 100;
  }

  char* end = nullptr;
  const auto source = std::string{value};
  const auto limit = std::strtol(source.c_str(), &end, 10);
  if (end == nullptr || *end != '\0' || limit < 1 || limit > 500) {
    return std::nullopt;
  }
  return static_cast<std::int32_t>(limit);
}

}  // namespace

NotificationDeliveriesHandler::NotificationDeliveriesHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      notifications_service_(
          component_context.FindComponent<NotificationsServiceComponent>()
              .GetService()) {}

std::string NotificationDeliveriesHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  if (request.GetMethod() != userver::server::http::HttpMethod::kGet) {
    return ErrorResponse(request,
                         userver::server::http::HttpStatus::kMethodNotAllowed,
                         "method is not allowed");
  }

  const auto limit = ParseLimit(request.GetArg("limit"));
  if (!limit) {
    return ErrorResponse(request,
                         userver::server::http::HttpStatus::kBadRequest,
                         "limit must be between 1 and 500");
  }

  try {
    return JsonResponse(
        request,
        SerializeNotificationDeliveries(
            notifications_service_.ListNotificationDeliveries(*limit)));
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "notification-service", ex);
  }
}

}  // namespace netwatch::api_gateway::notifications
