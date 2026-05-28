#pragma once

#include <string>
#include <string_view>
#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <checks/client/check_client.hpp>
#include <targets/client/target_client.hpp>

namespace monitor_service::checks {
class TargetStatusHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-target-status";

  TargetStatusHandler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  const CheckClient& check_client_;
  const target::TargetClient& target_client_;
};
}  // namespace monitor_service::checks
