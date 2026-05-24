#include "targets_handler.hpp"

#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/component.hpp>

#include "target_json.hpp"
#include "target_validator.hpp"

namespace monitor_service::target {

namespace {

std::string JsonResponse(const userver::server::http::HttpRequest& request,
                         const userver::formats::json::Value& body) {
  request.GetHttpResponse().SetContentType(
      userver::http::content_type::kApplicationJson);
  return userver::formats::json::ToString(body);
}

std::string BadRequest(const userver::server::http::HttpRequest& request,
                       std::string_view message) {
  request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
  return JsonResponse(request, SerializeError(message));
}

}  // namespace

TargetsHandler::TargetsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      repository_(
          component_context
              .FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

std::string TargetsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto method = request.GetMethod();

  if (method == userver::server::http::HttpMethod::kGet) {
    return HandleListTargets(request);
  }

  if (method != userver::server::http::HttpMethod::kPost) {
    request.SetResponseStatus(
        userver::server::http::HttpStatus::kMethodNotAllowed);
    return JsonResponse(request, SerializeError("method is not allowed"));
  }

  return HandleCreateTarget(request);
}

std::string TargetsHandler::HandleCreateTarget(
    const userver::server::http::HttpRequest& request) const {
  try {
    const auto request_json =
        userver::formats::json::FromString(request.RequestBody());
    const auto create_request = ParseCreateTargetRequest(request_json);
    if (const auto validation_error =
            monitor_service::target_validator::ValidateCreateTargetRequest(
                create_request)) {
      return BadRequest(request, *validation_error);
    }

    const auto target = repository_.CreateTarget(create_request);
    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    return JsonResponse(request, SerializeTarget(target));
  } catch (const userver::formats::json::Exception& ex) {
    return BadRequest(request, ex.what());
  } catch (const std::invalid_argument& ex) {
    return BadRequest(request, ex.what());
  }
}

std::string TargetsHandler::HandleListTargets(
    const userver::server::http::HttpRequest& request) const {
  return JsonResponse(request,
                      SerializeTargets(repository_.ListActiveTargets()));
}

}  // namespace monitor_service::target
