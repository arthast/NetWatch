#include <targets/handlers/target_by_id_handler.hpp>

#include <cstdint>
#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <common/http_response.hpp>
#include <common/path_params.hpp>
#include <targets/json/target_json.hpp>
#include <targets/validation/target_validator.hpp>

namespace monitor_service::target {
using common::ErrorResponse;
using common::JsonResponse;

namespace {
bool HasPatchFields(const UpdateTargetRequest& request) {
  return request.name || request.type || request.url || request.method ||
         request.expected_status_code || request.host || request.port ||
         request.interval_seconds || request.timeout_ms;
}

Target ApplyUpdate(Target target, const UpdateTargetRequest& request) {
  const auto previous_type = target.type;
  if (request.type) {
    target.type = *request.type;
  }

  if (target.type != previous_type) {
    if (target.type == TargetType::kHttp) {
      target.url = std::nullopt;
      target.method = "GET";
      target.expected_status_code = 200;
      target.host = std::nullopt;
      target.port = std::nullopt;
    } else {
      target.url = std::nullopt;
      target.method = std::nullopt;
      target.expected_status_code = std::nullopt;
      target.host = std::nullopt;
      target.port = std::nullopt;
    }
  }

  if (request.name) {
    target.name = *request.name;
  }
  if (request.url) {
    target.url = *request.url;
  }
  if (request.method) {
    target.method = *request.method;
  }
  if (request.expected_status_code) {
    target.expected_status_code = *request.expected_status_code;
  }
  if (request.host) {
    target.host = *request.host;
  }
  if (request.port) {
    target.port = *request.port;
  }
  if (request.interval_seconds) {
    target.interval_seconds = *request.interval_seconds;
  }
  if (request.timeout_ms) {
    target.timeout_ms = *request.timeout_ms;
  }

  return target;
}
}  // namespace

TargetByIdHandler::TargetByIdHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      target_client_(component_context.FindComponent<TargetClient>()) {}

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
  const auto target = target_client_.GetTargetById(target_id);
  if (!target) {
    return ErrorResponse(request, userver::server::http::HttpStatus::kNotFound,
                         "target not found");
  }

  return JsonResponse(request, SerializeTarget(*target));
}

std::string TargetByIdHandler::HandlePatchTarget(
    const userver::server::http::HttpRequest& request,
    std::int64_t target_id) const {
  try {
    const auto current_target = target_client_.GetTargetById(target_id);
    if (!current_target) {
      return ErrorResponse(request,
                           userver::server::http::HttpStatus::kNotFound,
                           "target not found");
    }

    const auto request_json =
        userver::formats::json::FromString(request.RequestBody());
    const auto update_request = ParseUpdateTargetRequest(request_json);
    if (!HasPatchFields(update_request)) {
      return ErrorResponse(request,
                           userver::server::http::HttpStatus::kBadRequest,
                           "patch body must contain at least one field");
    }

    const auto updated_target = ApplyUpdate(*current_target, update_request);
    if (const auto validation_error =
            monitor_service::target_validator::ValidateTarget(updated_target)) {
      return ErrorResponse(request,
                           userver::server::http::HttpStatus::kBadRequest,
                           *validation_error);
    }

    const auto saved_target = target_client_.UpdateTarget(updated_target);
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
  }
}

std::string TargetByIdHandler::HandleDeleteTarget(
    const userver::server::http::HttpRequest& request,
    std::int64_t target_id) const {
  if (!target_client_.DeactivateTarget(target_id)) {
    return ErrorResponse(request, userver::server::http::HttpStatus::kNotFound,
                         "target not found");
  }

  request.SetResponseStatus(userver::server::http::HttpStatus::kNoContent);
  return {};
}
}  // namespace monitor_service::target
