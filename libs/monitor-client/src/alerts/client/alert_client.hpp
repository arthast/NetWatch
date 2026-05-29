#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include <netwatch/monitor_service_client.usrv.pb.hpp>
#include <userver/components/component_base.hpp>

#include <alerts/model/alert.hpp>
#include <checks/model/check_result.hpp>
#include <targets/model/target.hpp>

namespace monitor_service::alerts {

class AlertClient final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "alert-client";

  AlertClient(const userver::components::ComponentConfig& config,
              const userver::components::ComponentContext& context);

  std::vector<Alert> ListAlerts() const;

  std::vector<Alert> ListActiveAlerts() const;

  void ProcessCheckResult(
      const target::Target& target,
      const std::optional<checks::CheckResult>& previous_check,
      const checks::CheckResult& current_check) const;

 private:
  netwatch::monitor::v1::AlertServiceClient& client_;
};

}  // namespace monitor_service::alerts
