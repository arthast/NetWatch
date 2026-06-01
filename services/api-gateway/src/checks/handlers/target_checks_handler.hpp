#pragma once

#include <string>
#include <string_view>
#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <monitor_client/client/check_client.hpp>

namespace netwatch::api_gateway::checks {
class TargetChecksHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-target-checks";

  TargetChecksHandler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  const netwatch::monitor_client::CheckClient& check_client_;
};
}  // namespace netwatch::api_gateway::checks
