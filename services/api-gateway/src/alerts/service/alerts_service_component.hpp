#pragma once

#include <string_view>
#include <userver/components/component_base.hpp>

#include <alerts/service/alerts_service.hpp>

namespace netwatch::api_gateway::alerts {

class AlertsServiceComponent final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "alerts-service";

  AlertsServiceComponent(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  const AlertsService& GetService() const;

 private:
  AlertsService alerts_service_;
};

}  // namespace netwatch::api_gateway::alerts
