#pragma once

#include <string_view>
#include <userver/components/component_base.hpp>

#include <notifications/service/notifications_service.hpp>

namespace netwatch::api_gateway::notifications {

class NotificationsServiceComponent final
    : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "notifications-service";

  NotificationsServiceComponent(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  const NotificationsService& GetService() const;

 private:
  NotificationsService notifications_service_;
};

}  // namespace netwatch::api_gateway::notifications
