#pragma once

#include <string>
#include <string_view>
#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <checks/storage/check_repository.hpp>
#include <targets/storage/target_repository.hpp>

namespace monitor_service::checks {
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
  target::TargetRepository target_repository_;
  CheckRepository check_repository_;
};
}  // namespace monitor_service::checks
