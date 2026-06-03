#pragma once

#include <string_view>
#include <userver/components/component_base.hpp>

#include <checks/service/checks_service.hpp>

namespace netwatch::api_gateway::checks {

class ChecksServiceComponent final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "checks-service";

  ChecksServiceComponent(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  const ChecksService& GetService() const;

 private:
  ChecksService checks_service_;
};

}  // namespace netwatch::api_gateway::checks
