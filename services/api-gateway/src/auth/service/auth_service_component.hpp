#pragma once

#include <memory>
#include <string_view>
#include <userver/components/component.hpp>

#include <auth/service/auth_service.hpp>

namespace netwatch::api_gateway::auth {

class AuthServiceComponent final
    : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "auth-service";

  AuthServiceComponent(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  const AuthService& GetService() const;

 private:
  std::unique_ptr<AuthService> service_;
};

}  // namespace netwatch::api_gateway::auth
