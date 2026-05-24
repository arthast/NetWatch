#pragma once

#include "target_repository.hpp"

#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>

#include <string_view>

namespace monitor_service::target {

class TargetsHandler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-targets";

    TargetsHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& component_context
    );

    userver::formats::json::Value HandleRequestJsonThrow(
        const userver::server::http::HttpRequest& request,
        const userver::formats::json::Value& request_json,
        userver::server::request::RequestContext& context
    ) const override;

private:
    TargetRepository repository_;
};

}  // namespace monitor_service::target
