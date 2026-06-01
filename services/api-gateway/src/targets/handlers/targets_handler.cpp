#include <targets/handlers/targets_handler.hpp>

#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

#include <common/http_response.hpp>
#include <targets/json/target_json.hpp>

namespace netwatch::api_gateway::targets {
using common::ErrorResponse;
using common::JsonResponse;

TargetsHandler::TargetsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      target_client_(
          component_context
              .FindComponent<netwatch::target_client::TargetClient>()) {}

std::string TargetsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  const auto method = request.GetMethod();

  if (method == userver::server::http::HttpMethod::kGet) {
    return HandleListTargets(request);
  }

  if (method != userver::server::http::HttpMethod::kPost) {
    return ErrorResponse(request,
                         userver::server::http::HttpStatus::kMethodNotAllowed,
                         "method is not allowed");
  }

  return HandleCreateTarget(request);
}

std::string TargetsHandler::HandleCreateTarget(
    const userver::server::http::HttpRequest& request) const {
  try {
    const auto request_json =
        userver::formats::json::FromString(request.RequestBody());
    const auto create_request = ParseCreateTargetRequest(request_json);

    const auto target = target_client_.CreateTarget(create_request);
    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    return JsonResponse(request, SerializeTarget(target));
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

std::string TargetsHandler::HandleListTargets(
    const userver::server::http::HttpRequest& request) const {
  try {
    return JsonResponse(request,
                        SerializeTargets(target_client_.ListActiveTargets()));
  } catch (const userver::ugrpc::client::BaseError& ex) {
    return common::UpstreamErrorResponse(request, "target-service", ex);
  }
}
}  // namespace netwatch::api_gateway::targets
