#pragma once

#include <string>
#include <string_view>
#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "target_repository.hpp"

namespace monitor_service::target {
class TargetByIdHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-target-by-id";

    TargetByIdHandler(
        const userver::components::ComponentConfig &config,
        const userver::components::ComponentContext &component_context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest &request,
        userver::server::request::RequestContext &context) const override;

private:
    TargetRepository repository_;
};
} // namespace monitor_service::target
