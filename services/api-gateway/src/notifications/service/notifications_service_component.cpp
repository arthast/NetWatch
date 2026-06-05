#include <notifications/service/notifications_service_component.hpp>

#include <userver/components/component_context.hpp>

namespace netwatch::api_gateway::notifications {

NotificationsServiceComponent::NotificationsServiceComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : ComponentBase(config, component_context),
      notifications_service_(
          component_context
              .FindComponent<
                  netwatch::notification_client::NotificationClient>()) {}

const NotificationsService& NotificationsServiceComponent::GetService()
    const {
  return notifications_service_;
}

}  // namespace netwatch::api_gateway::notifications
