#pragma once

#include <string>
#include <string_view>
#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <auth/service/auth_service.hpp>
#include <targets/service/targets_service.hpp>

namespace netwatch::api_gateway::targets {
class TargetByIdHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-target-by-id";

  TargetByIdHandler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  std::string HandleGetTarget(const userver::server::http::HttpRequest& request,
                              std::int64_t target_id) const;

  std::string HandlePatchTarget(
      const userver::server::http::HttpRequest& request,
      std::int64_t target_id) const;

  std::string HandleDeleteTarget(
      const userver::server::http::HttpRequest& request,
      std::int64_t target_id) const;

  const TargetsService& targets_service_;
  const auth::AuthService& auth_service_;
};
}  // namespace netwatch::api_gateway::targets
