#pragma once

#include <optional>

#include <alert_client/client/alert_client.hpp>
#include <checks/model/check_result.hpp>
#include <target_client/model/target.hpp>

namespace netwatch::monitor_service::checks {

class CheckAlertNotifier final {
 public:
  explicit CheckAlertNotifier(
      const netwatch::alert_client::AlertClient& alert_client);

  void ProcessCheckResult(const netwatch::target_client::Target& target,
                          const std::optional<CheckResult>& previous_check,
                          const CheckResult& current_check) const;

 private:
  const netwatch::alert_client::AlertClient& alert_client_;
};

}  // namespace netwatch::monitor_service::checks
