#pragma once

#include <string>
#include <string_view>
#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <targets/client/target_client.hpp>

namespace netwatch::api_gateway::targets {
class TargetsHandler final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-targets";

  TargetsHandler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  std::string HandleCreateTarget(
      const userver::server::http::HttpRequest& request) const;

  std::string HandleListTargets(
      const userver::server::http::HttpRequest& request) const;

  const netwatch::target_client::TargetClient& target_client_;
};
}  // namespace netwatch::api_gateway::targets
