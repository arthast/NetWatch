#include "targets_handler.hpp"

#include "target_json.hpp"
#include "target_validator.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/component.hpp>

#include <stdexcept>

namespace monitor_service::target {

namespace {

userver::formats::json::Value BadRequest(
    const userver::server::http::HttpRequest& request,
    std::string_view message
) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
    return SerializeError(message);
}

}  // namespace

TargetsHandler::TargetsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context),
      repository_(
          component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()
      ) {}

userver::formats::json::Value TargetsHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& request_json,
    userver::server::request::RequestContext&
) const {
    try {
        const auto create_request = ParseCreateTargetRequest(request_json);
        if (const auto validation_error =
                monitor_service::target_validator::ValidateCreateTargetRequest(create_request)) {
            return BadRequest(request, *validation_error);
        }

        const auto target = repository_.CreateTarget(create_request);
        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return SerializeTarget(target);
    } catch (const userver::formats::json::Exception& ex) {
        return BadRequest(request, ex.what());
    } catch (const std::invalid_argument& ex) {
        return BadRequest(request, ex.what());
    }
}

}  // namespace monitor_service::target
