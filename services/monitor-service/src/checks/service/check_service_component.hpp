#pragma once

#include <string_view>
#include <userver/components/component_base.hpp>

#include <checks/service/check_service.hpp>

namespace netwatch::monitor_service::checks {

class CheckServiceComponent final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "check-service";

  CheckServiceComponent(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  const CheckService& GetService() const;

 private:
  CheckService check_service_;
};

}  // namespace netwatch::monitor_service::checks
