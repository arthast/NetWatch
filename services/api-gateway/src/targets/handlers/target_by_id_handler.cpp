#include <targets/handlers/target_by_id_handler.hpp>

#include <cstdint>
#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <auth/service/auth_service_component.hpp>
#include <common/auth.hpp>
#include <common/http_response.hpp>
#include <common/path_params.hpp>
#include <targets/json/target_json.hpp>
#include <targets/service/targets_service_component.hpp>

namespace netwatch::api_gateway::targets {
using common::ErrorResponse;
using common::JsonResponse;

TargetByIdHandler::TargetByIdHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      targets_service_(
          component_context.FindComponent<TargetsServiceComponent>()
              .GetService()),
      auth_service_(
          component_context.FindComponent<auth::AuthServiceComponent>()
              .GetService()) {}

std::string TargetByIdHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto target_id = common::ParsePositiveInt64(request.GetPathArg("id"));
  if (!target_id) {
    return ErrorResponse(request,
                         userver::server::http::HttpStatus::kBadRequest,
                         "target id must be a positive integer");
  }

  const auto method = request.GetMethod();
  if (method == userver::server::http::HttpMethod::kGet) {
    return HandleGetTarget(request, *target_id);
  }
  if (method == userver::server::http::HttpMethod::kPatch) {
    return HandlePatchTarget(request, *target_id);
  }
  if (method == userver::server::http::HttpMethod::kDelete) {
    return HandleDeleteTarget(request, *target_id);
  }

  return ErrorResponse(request,
                       userver::server::http::HttpStatus::kMethodNotAllowed,
                       "method is not allowed");
}

std::string TargetByIdHandler::HandleGetTarget(
    const userver::server::http::HttpRequest& request,
    std::int64_t target_id) const {
  try {
    const auto session = common::AuthenticateRequest(request, auth_service_);
    if (!session) {
      return ErrorResponse(request,
                           userver::server::http::HttpStatus::kUnauthorized,
                           "Authorization bearer token is required");
    }

    const auto target =
        targets_service_.GetTargetById(session->user.id, target_id);
    if (!target) {
      return ErrorResponse(request,
                           userver::server::http::HttpStatus::kNotFound,
                           "target not found");
    }

    return JsonResponse(request, SerializeTarget(*target));
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "target-service", ex);
  }
}

std::string TargetByIdHandler::HandlePatchTarget(
    const userver::server::http::HttpRequest& request,
    std::int64_t target_id) const {
  try {
    const auto session = common::AuthenticateRequest(request, auth_service_);
    if (!session) {
      return ErrorResponse(request,
                           userver::server::http::HttpStatus::kUnauthorized,
                           "Authorization bearer token is required");
    }

    const auto request_json =
        userver::formats::json::FromString(request.RequestBody());
    const auto update_request = ParseUpdateTargetRequest(request_json);

    const auto saved_target = targets_service_.UpdateTarget(
        session->user.id, target_id, update_request);
    if (!saved_target) {
      return ErrorResponse(request,
                           userver::server::http::HttpStatus::kNotFound,
                           "target not found");
    }

    return JsonResponse(request, SerializeTarget(*saved_target));
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

std::string TargetByIdHandler::HandleDeleteTarget(
    const userver::server::http::HttpRequest& request,
    std::int64_t target_id) const {
  try {
    const auto session = common::AuthenticateRequest(request, auth_service_);
    if (!session) {
      return ErrorResponse(request,
                           userver::server::http::HttpStatus::kUnauthorized,
                           "Authorization bearer token is required");
    }

    if (!targets_service_.DeactivateTarget(session->user.id, target_id)) {
      return ErrorResponse(request,
                           userver::server::http::HttpStatus::kNotFound,
                           "target not found");
    }

    request.SetResponseStatus(userver::server::http::HttpStatus::kNoContent);
    return {};
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "target-service", ex);
  }
}
}  // namespace netwatch::api_gateway::targets
