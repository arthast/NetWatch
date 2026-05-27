#pragma once

#include <string>
#include <string_view>
#include <userver/server/handlers/http_handler_base.hpp>

namespace monitor_service::web {

class SwaggerUiHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-swagger-ui";

  SwaggerUiHandler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;
};

}  // namespace monitor_service::web
