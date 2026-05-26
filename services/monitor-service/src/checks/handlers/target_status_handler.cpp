#include <checks/handlers/target_status_handler.hpp>

#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>
#include <system_error>
#include <userver/components/component_context.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/component.hpp>

#include <checks/json/check_json.hpp>
#include <common/http_response.hpp>

namespace monitor_service::checks {
namespace {
    std::optional<std::int64_t> ParseTargetId(std::string_view value) {
        std::int64_t id = 0;
        const auto *begin = value.data();
        const auto *end = value.data() + value.size();
        const auto [ptr, error] = std::from_chars(begin, end, id);
        if (error != std::errc{} || ptr != end || id <= 0) {
            return std::nullopt;
        }

        return id;
    }
} // namespace

TargetStatusHandler::TargetStatusHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &component_context)
    : HttpHandlerBase(config, component_context),
      target_repository_(
          component_context
          .FindComponent<userver::components::Postgres>("postgres-db-1")
          .GetCluster()),
      check_repository_(
          component_context
          .FindComponent<userver::components::Postgres>("postgres-db-1")
          .GetCluster()) {
}

std::string TargetStatusHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest &request,
    userver::server::request::RequestContext &) const {
    const auto target_id = ParseTargetId(request.GetPathArg("id"));
    if (!target_id) {
        return common::ErrorResponse(
            request, userver::server::http::HttpStatus::kBadRequest,
            "target id must be a positive integer");
    }

    if (!target_repository_.GetTargetById(*target_id)) {
        return common::ErrorResponse(
            request, userver::server::http::HttpStatus::kNotFound,
            "target not found");
    }

    const auto status = check_repository_.GetLatestTargetStatus(*target_id);
    if (!status) {
        return common::ErrorResponse(
            request, userver::server::http::HttpStatus::kNotFound,
            "target has no checks");
    }

    return common::JsonResponse(request, SerializeCheckResult(*status));
}
} // namespace monitor_service::checks
