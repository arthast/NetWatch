#include <alerts/service/alert_service.hpp>

#include <string>
#include <utility>

namespace netwatch::alert_service::alerts {
namespace {

std::string TargetTypeToString(TargetType type) {
  switch (type) {
    case TargetType::kHttp:
      return "http";
    case TargetType::kTcp:
      return "tcp";
  }

  return "http";
}

AlertEventTargetSnapshot ToAlertEventTarget(const TargetSnapshot& target) {
  return AlertEventTargetSnapshot{
      .id = target.id,
      .user_id = target.user_id,
      .name = target.name,
      .type = TargetTypeToString(target.type),
  };
}

}  // namespace

AlertService::AlertService(AlertRepository repository)
    : repository_(std::move(repository)) {}

void AlertService::ProcessCheckResult(
    const TargetSnapshot& target,
    const std::optional<CheckResultSnapshot>& previous_check,
    const CheckResultSnapshot& current_check) const {
  if (current_check.status == CheckStatus::kDown) {
    ProcessTargetDown(target);
    return;
  }

  if (!previous_check || previous_check->status == CheckStatus::kDown) {
    ProcessTargetRecovered(target);
  }
}

void AlertService::ProcessTargetDown(const TargetSnapshot& target) const {
  if (repository_.FindActiveAlert(target.id, AlertType::kTargetDown)) {
    return;
  }

  repository_.CreateAlertWithEvent(
      NewAlert{
          .target_id = target.id,
          .type = AlertType::kTargetDown,
          .severity = AlertSeverity::kCritical,
          .message = "Target " + target.name + " is down",
      },
      ToAlertEventTarget(target));
}

void AlertService::ProcessTargetRecovered(const TargetSnapshot& target) const {
  repository_.ResolveActiveAlertWithEvent(target.id, AlertType::kTargetDown,
                                          ToAlertEventTarget(target));
}

}  // namespace netwatch::alert_service::alerts
