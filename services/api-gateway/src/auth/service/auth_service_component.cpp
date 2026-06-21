#include <auth/service/auth_service_component.hpp>

#include <userver/components/component_context.hpp>

namespace netwatch::api_gateway::auth {

AuthServiceComponent::AuthServiceComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : ComponentBase(config, component_context),
      service_(std::make_unique<AuthService>(
          component_context.FindComponent<auth_client::AuthClient>())) {}

const AuthService& AuthServiceComponent::GetService() const {
  return *service_;
}

}  // namespace netwatch::api_gateway::auth
