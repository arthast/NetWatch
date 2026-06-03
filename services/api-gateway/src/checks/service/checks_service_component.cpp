#include <checks/service/checks_service_component.hpp>

#include <userver/components/component_context.hpp>

namespace netwatch::api_gateway::checks {

ChecksServiceComponent::ChecksServiceComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : ComponentBase(config, component_context),
      checks_service_(
          component_context
              .FindComponent<netwatch::monitor_client::CheckClient>(),
          component_context
              .FindComponent<netwatch::target_client::TargetClient>()) {}

const ChecksService& ChecksServiceComponent::GetService() const {
  return checks_service_;
}

}  // namespace netwatch::api_gateway::checks
