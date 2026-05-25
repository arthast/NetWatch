#include <targets/handlers/targets_handler.hpp>

#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/component.hpp>

#include <targets/handlers/target_http_response.hpp>
#include <targets/json/target_json.hpp>
#include <targets/validation/target_validator.hpp>

namespace monitor_service::target {

TargetsHandler::TargetsHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &component_context)
    : HttpHandlerBase(config, component_context),
      repository_(
          component_context
          .FindComponent<userver::components::Postgres>("postgres-db-1")
          .GetCluster()) {
}

std::string TargetsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest &request,
    userver::server::request::RequestContext &) const {
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
    const userver::server::http::HttpRequest &request) const {
    try {
        const auto request_json =
                userver::formats::json::FromString(request.RequestBody());
        const auto create_request = ParseCreateTargetRequest(request_json);
        if (const auto validation_error =
                monitor_service::target_validator::ValidateCreateTargetRequest(
                    create_request)) {
            return ErrorResponse(request,
                                 userver::server::http::HttpStatus::kBadRequest,
                                 *validation_error);
        }

        const auto target = repository_.CreateTarget(create_request);
        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return JsonResponse(request, SerializeTarget(target));
    } catch (const userver::formats::json::Exception &ex) {
        return ErrorResponse(request, userver::server::http::HttpStatus::kBadRequest,
                             ex.what());
    } catch (const std::invalid_argument &ex) {
        return ErrorResponse(request, userver::server::http::HttpStatus::kBadRequest,
                             ex.what());
    }
}

std::string TargetsHandler::HandleListTargets(
    const userver::server::http::HttpRequest &request) const {
    return JsonResponse(request, SerializeTargets(repository_.ListActiveTargets()));
}
} // namespace monitor_service::target
