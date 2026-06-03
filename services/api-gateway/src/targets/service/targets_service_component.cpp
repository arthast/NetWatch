#include <targets/service/targets_service_component.hpp>

#include <userver/components/component_context.hpp>

namespace netwatch::api_gateway::targets {

TargetsServiceComponent::TargetsServiceComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : ComponentBase(config, component_context),
      targets_service_(
          component_context
              .FindComponent<netwatch::target_client::TargetClient>()) {}

const TargetsService& TargetsServiceComponent::GetService() const {
  return targets_service_;
}

}  // namespace netwatch::api_gateway::targets
