#include <alerts/grpc/alert_grpc_service.hpp>

#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/component.hpp>
#include <vector>

namespace monitor_service::alerts {
namespace {

namespace proto = netwatch::monitor::v1;

proto::AlertType ToProtoAlertType(AlertType type) {
  switch (type) {
    case AlertType::kTargetDown:
      return proto::ALERT_TYPE_TARGET_DOWN;
    case AlertType::kTargetRecovered:
      return proto::ALERT_TYPE_TARGET_RECOVERED;
    case AlertType::kHighLatency:
      return proto::ALERT_TYPE_HIGH_LATENCY;
  }
  return proto::ALERT_TYPE_UNSPECIFIED;
}

proto::AlertSeverity ToProtoAlertSeverity(AlertSeverity severity) {
  switch (severity) {
    case AlertSeverity::kWarning:
      return proto::ALERT_SEVERITY_WARNING;
    case AlertSeverity::kCritical:
      return proto::ALERT_SEVERITY_CRITICAL;
  }
  return proto::ALERT_SEVERITY_UNSPECIFIED;
}

void FillProtoAlert(const Alert& source, proto::Alert& target) {
  target.set_id(source.id);
  target.set_target_id(source.target_id);
  target.set_type(ToProtoAlertType(source.type));
  target.set_severity(ToProtoAlertSeverity(source.severity));
  target.set_message(source.message);
  target.set_created_at(source.created_at);

  if (source.resolved_at) {
    target.set_resolved_at(*source.resolved_at);
  }
}

proto::ListAlertsResponse MakeAlertsResponse(const std::vector<Alert>& alerts) {
  proto::ListAlertsResponse response;
  for (const auto& alert : alerts) {
    FillProtoAlert(alert, *response.add_alerts());
  }
  return response;
}

}  // namespace

AlertGrpcService::AlertGrpcService(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : proto::AlertServiceBase::Component(config, context),
      repository_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

AlertGrpcService::ListAlertsResult AlertGrpcService::ListAlerts(
    CallContext&, proto::ListAlertsRequest&&) {
  return MakeAlertsResponse(repository_.ListAlerts());
}

AlertGrpcService::ListActiveAlertsResult AlertGrpcService::ListActiveAlerts(
    CallContext&, proto::ListAlertsRequest&&) {
  return MakeAlertsResponse(repository_.ListActiveAlerts());
}

}  // namespace monitor_service::alerts
