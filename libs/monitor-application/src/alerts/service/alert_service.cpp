#include <alerts/service/alert_service.hpp>

#include <string>
#include <utility>

namespace monitor_service::alerts {

AlertService::AlertService(AlertRepository repository)
    : repository_(std::move(repository)) {}

void AlertService::ProcessCheckResult(
    const target::Target& target,
    const std::optional<checks::CheckResult>& previous_check,
    const checks::CheckResult& current_check) const {
  if (current_check.status == checks::CheckStatus::kDown) {
    ProcessTargetDown(target);
    return;
  }

  if (!previous_check || previous_check->status == checks::CheckStatus::kDown) {
    ProcessTargetRecovered(target);
  }
}

void AlertService::ProcessTargetDown(const target::Target& target) const {
  if (repository_.FindActiveAlert(target.id, AlertType::kTargetDown)) {
    return;
  }

  repository_.CreateAlert(NewAlert{
      .target_id = target.id,
      .type = AlertType::kTargetDown,
      .severity = AlertSeverity::kCritical,
      .message = "Target " + target.name + " is down",
  });
}

void AlertService::ProcessTargetRecovered(const target::Target& target) const {
  repository_.ResolveActiveAlert(target.id, AlertType::kTargetDown);
}

}  // namespace monitor_service::alerts
