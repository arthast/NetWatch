#include <alerts/service/alerts_service_component.hpp>

#include <userver/components/component_context.hpp>

namespace netwatch::api_gateway::alerts {

AlertsServiceComponent::AlertsServiceComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : ComponentBase(config, component_context),
      alerts_service_(
          component_context
              .FindComponent<netwatch::alert_client::AlertClient>()) {}

const AlertsService& AlertsServiceComponent::GetService() const {
  return alerts_service_;
}

}  // namespace netwatch::api_gateway::alerts
