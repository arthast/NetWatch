#include "target_by_id_handler.hpp"

#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>
#include <system_error>
#include <userver/components/component_context.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/component.hpp>

#include "target_http_response.hpp"
#include "target_json.hpp"

namespace monitor_service::target {
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

TargetByIdHandler::TargetByIdHandler(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &component_context)
    : HttpHandlerBase(config, component_context),
      repository_(
          component_context
          .FindComponent<userver::components::Postgres>("postgres-db-1")
          .GetCluster()) {
}

std::string TargetByIdHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest &request,
    userver::server::request::RequestContext &) const {
    const auto target_id = ParseTargetId(request.GetPathArg("id"));
    if (!target_id) {
        return ErrorResponse(request,
                             userver::server::http::HttpStatus::kBadRequest,
                             "target id must be a positive integer");
    }

    const auto target = repository_.GetTargetById(*target_id);
    if (!target) {
        return ErrorResponse(request,
                             userver::server::http::HttpStatus::kNotFound,
                             "target not found");
    }

    return JsonResponse(request, SerializeTarget(*target));
}
} // namespace monitor_service::target
