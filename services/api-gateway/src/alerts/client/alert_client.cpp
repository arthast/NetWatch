#include <alerts/client/alert_client.hpp>

#include <chrono>
#include <optional>
#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/ugrpc/client/call_options.hpp>
#include <userver/ugrpc/client/simple_client_component.hpp>
#include <vector>

namespace monitor_service::alerts {
namespace {

namespace proto = netwatch::monitor::v1;

userver::ugrpc::client::CallOptions MakeCallOptions() {
  userver::ugrpc::client::CallOptions options;
  options.SetAttempts(1);
  options.SetTimeout(std::chrono::milliseconds{1000});
  return options;
}

AlertType ToDomainAlertType(proto::AlertType type) {
  switch (type) {
    case proto::ALERT_TYPE_TARGET_DOWN:
      return AlertType::kTargetDown;
    case proto::ALERT_TYPE_TARGET_RECOVERED:
      return AlertType::kTargetRecovered;
    case proto::ALERT_TYPE_HIGH_LATENCY:
      return AlertType::kHighLatency;
    case proto::ALERT_TYPE_UNSPECIFIED:
    case proto::AlertType_INT_MIN_SENTINEL_DO_NOT_USE_:
    case proto::AlertType_INT_MAX_SENTINEL_DO_NOT_USE_:
      break;
  }

  throw std::invalid_argument("unknown alert type");
}

AlertSeverity ToDomainAlertSeverity(proto::AlertSeverity severity) {
  switch (severity) {
    case proto::ALERT_SEVERITY_WARNING:
      return AlertSeverity::kWarning;
    case proto::ALERT_SEVERITY_CRITICAL:
      return AlertSeverity::kCritical;
    case proto::ALERT_SEVERITY_UNSPECIFIED:
    case proto::AlertSeverity_INT_MIN_SENTINEL_DO_NOT_USE_:
    case proto::AlertSeverity_INT_MAX_SENTINEL_DO_NOT_USE_:
      break;
  }

  throw std::invalid_argument("unknown alert severity");
}

Alert ToDomainAlert(const proto::Alert& alert) {
  return Alert{
      .id = alert.id(),
      .target_id = alert.target_id(),
      .type = ToDomainAlertType(alert.type()),
      .severity = ToDomainAlertSeverity(alert.severity()),
      .message = alert.message(),
      .created_at = alert.created_at(),
      .resolved_at = alert.has_resolved_at()
                         ? std::make_optional(alert.resolved_at())
                         : std::nullopt,
  };
}

std::vector<Alert> ToDomainAlerts(const proto::ListAlertsResponse& response) {
  std::vector<Alert> alerts;
  alerts.reserve(response.alerts_size());
  for (const auto& alert : response.alerts()) {
    alerts.push_back(ToDomainAlert(alert));
  }
  return alerts;
}

}  // namespace

AlertClient::AlertClient(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      client_(context
                  .FindComponent<userver::ugrpc::client::SimpleClientComponent<
                      proto::AlertServiceClient>>("alert-service-client")
                  .GetClient()) {}

std::vector<Alert> AlertClient::ListAlerts() const {
  return ToDomainAlerts(
      client_.ListAlerts(proto::ListAlertsRequest{}, MakeCallOptions()));
}

std::vector<Alert> AlertClient::ListActiveAlerts() const {
  return ToDomainAlerts(
      client_.ListActiveAlerts(proto::ListAlertsRequest{}, MakeCallOptions()));
}

}  // namespace monitor_service::alerts
