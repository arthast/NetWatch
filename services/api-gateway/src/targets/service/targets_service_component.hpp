#pragma once

#include <string_view>
#include <userver/components/component_base.hpp>

#include <targets/service/targets_service.hpp>

namespace netwatch::api_gateway::targets {

class TargetsServiceComponent final
    : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "targets-service";

  TargetsServiceComponent(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  const TargetsService& GetService() const;

 private:
  TargetsService targets_service_;
};

}  // namespace netwatch::api_gateway::targets
