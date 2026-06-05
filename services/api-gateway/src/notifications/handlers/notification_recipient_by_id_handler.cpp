#include <notifications/handlers/notification_recipient_by_id_handler.hpp>

#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <common/http_response.hpp>
#include <common/path_params.hpp>
#include <notifications/json/recipient_json.hpp>
#include <notifications/service/notifications_service_component.hpp>

namespace netwatch::api_gateway::notifications {
using common::ErrorResponse;
using common::JsonResponse;

NotificationRecipientByIdHandler::NotificationRecipientByIdHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      notifications_service_(
          component_context.FindComponent<NotificationsServiceComponent>()
              .GetService()) {}

std::string NotificationRecipientByIdHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto recipient_id =
      common::ParsePositiveInt64(request.GetPathArg("id"));
  if (!recipient_id) {
    return ErrorResponse(request,
                         userver::server::http::HttpStatus::kBadRequest,
                         "recipient id must be a positive integer");
  }

  const auto method = request.GetMethod();
  if (method == userver::server::http::HttpMethod::kGet) {
    return HandleGetRecipient(request, *recipient_id);
  }
  if (method == userver::server::http::HttpMethod::kPatch) {
    return HandlePatchRecipient(request, *recipient_id);
  }
  if (method == userver::server::http::HttpMethod::kDelete) {
    return HandleDeleteRecipient(request, *recipient_id);
  }

  return ErrorResponse(request,
                       userver::server::http::HttpStatus::kMethodNotAllowed,
                       "method is not allowed");
}

std::string NotificationRecipientByIdHandler::HandleGetRecipient(
    const userver::server::http::HttpRequest& request,
    std::int64_t recipient_id) const {
  try {
    const auto recipient =
        notifications_service_.GetEmailRecipient(recipient_id);
    if (!recipient) {
      return ErrorResponse(request,
                           userver::server::http::HttpStatus::kNotFound,
                           "email recipient not found");
    }

    return JsonResponse(request, SerializeEmailRecipient(*recipient));
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "notification-service", ex);
  }
}

std::string NotificationRecipientByIdHandler::HandlePatchRecipient(
    const userver::server::http::HttpRequest& request,
    std::int64_t recipient_id) const {
  try {
    const auto request_json =
        userver::formats::json::FromString(request.RequestBody());
    const auto update_request =
        ParseUpdateEmailRecipientRequest(request_json);

    const auto recipient =
        notifications_service_.UpdateEmailRecipient(recipient_id,
                                                    update_request);
    if (!recipient) {
      return ErrorResponse(request,
                           userver::server::http::HttpStatus::kNotFound,
                           "email recipient not found");
    }

    return JsonResponse(request, SerializeEmailRecipient(*recipient));
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

std::string NotificationRecipientByIdHandler::HandleDeleteRecipient(
    const userver::server::http::HttpRequest& request,
    std::int64_t recipient_id) const {
  try {
    if (!notifications_service_.DeleteEmailRecipient(recipient_id)) {
      return ErrorResponse(request,
                           userver::server::http::HttpStatus::kNotFound,
                           "email recipient not found");
    }

    request.SetResponseStatus(userver::server::http::HttpStatus::kNoContent);
    return {};
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "notification-service", ex);
  }
}

}  // namespace netwatch::api_gateway::notifications
