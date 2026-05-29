#pragma once

#include <optional>

#include <alerts/storage/alert_repository.hpp>
#include <checks/model/check_result.hpp>
#include <targets/model/target.hpp>

namespace monitor_service::alerts {

class AlertService {
 public:
  explicit AlertService(AlertRepository repository);

  void ProcessCheckResult(
      const target::Target& target,
      const std::optional<checks::CheckResult>& previous_check,
      const checks::CheckResult& current_check) const;

 private:
  void ProcessTargetDown(const target::Target& target) const;

  void ProcessTargetRecovered(const target::Target& target) const;

  AlertRepository repository_;
};

}  // namespace monitor_service::alerts
