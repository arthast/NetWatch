#include <alert_client/client/alert_client.hpp>

#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/ugrpc/client/simple_client_component.hpp>
#include <vector>

#include <client_common/call_options.hpp>

namespace netwatch::alert_client {
namespace {

namespace proto = netwatch::alert::v1;

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

proto::CheckStatus ToProtoCheckStatus(CheckStatus status) {
  switch (status) {
    case CheckStatus::kUp:
      return proto::CHECK_STATUS_UP;
    case CheckStatus::kDown:
      return proto::CHECK_STATUS_DOWN;
  }
  return proto::CHECK_STATUS_UNSPECIFIED;
}

proto::TargetType ToProtoTargetType(TargetType type) {
  switch (type) {
    case TargetType::kHttp:
      return proto::TARGET_TYPE_HTTP;
    case TargetType::kTcp:
      return proto::TARGET_TYPE_TCP;
  }
  return proto::TARGET_TYPE_UNSPECIFIED;
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

void FillProtoTarget(const TargetSnapshot& source,
                     proto::TargetSnapshot& result) {
  result.set_id(source.id);
  if (source.user_id) {
    result.set_user_id(*source.user_id);
  }
  result.set_name(source.name);
  result.set_type(ToProtoTargetType(source.type));

  if (source.url) {
    result.set_url(*source.url);
  }
  if (source.method) {
    result.set_method(*source.method);
  }
  if (source.expected_status_code) {
    result.set_expected_status_code(*source.expected_status_code);
  }
  if (source.host) {
    result.set_host(*source.host);
  }
  if (source.port) {
    result.set_port(*source.port);
  }

  result.set_interval_seconds(source.interval_seconds);
  result.set_timeout_ms(source.timeout_ms);
  result.set_is_active(source.is_active);
}

void FillProtoCheck(const CheckResultSnapshot& source,
                    proto::CheckResultSnapshot& result) {
  result.set_id(source.id);
  result.set_target_id(source.target_id);
  result.set_status(ToProtoCheckStatus(source.status));
  result.set_protocol(ToProtoTargetType(source.protocol));

  if (source.http_status) {
    result.set_http_status(*source.http_status);
  }
  if (source.latency_ms) {
    result.set_latency_ms(*source.latency_ms);
  }
  if (source.error_message) {
    result.set_error_message(*source.error_message);
  }

  result.set_checked_at(source.checked_at);
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
  return ToDomainAlerts(client_.ListAlerts(
      proto::ListAlertsRequest{}, client_common::MakeGrpcCallOptions()));
}

std::vector<Alert> AlertClient::ListActiveAlerts() const {
  return ToDomainAlerts(client_.ListActiveAlerts(
      proto::ListAlertsRequest{}, client_common::MakeGrpcCallOptions()));
}

void AlertClient::ProcessCheckResult(
    const TargetSnapshot& target,
    const std::optional<CheckResultSnapshot>& previous_check,
    const CheckResultSnapshot& current_check) const {
  proto::ProcessCheckResultRequest request;
  FillProtoTarget(target, *request.mutable_target());
  if (previous_check) {
    FillProtoCheck(*previous_check, *request.mutable_previous_check());
  }
  FillProtoCheck(current_check, *request.mutable_current_check());

  client_.ProcessCheckResult(request, client_common::MakeGrpcCallOptions());
}

}  // namespace netwatch::alert_client
