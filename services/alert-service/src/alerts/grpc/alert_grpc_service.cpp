#include <alerts/grpc/alert_grpc_service.hpp>

#include <grpcpp/support/status.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/component.hpp>
#include <utility>
#include <vector>

namespace netwatch::alert_service::alerts {
namespace {

namespace proto = netwatch::alert::v1;

grpc::Status InvalidArgument(std::string message) {
  return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, std::move(message)};
}

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

TargetType ToDomainTargetType(proto::TargetType type) {
  switch (type) {
    case proto::TARGET_TYPE_HTTP:
      return TargetType::kHttp;
    case proto::TARGET_TYPE_TCP:
      return TargetType::kTcp;
    case proto::TARGET_TYPE_UNSPECIFIED:
    case proto::TargetType_INT_MIN_SENTINEL_DO_NOT_USE_:
    case proto::TargetType_INT_MAX_SENTINEL_DO_NOT_USE_:
      break;
  }

  throw std::invalid_argument("target type must be http or tcp");
}

CheckStatus ToDomainCheckStatus(proto::CheckStatus status) {
  switch (status) {
    case proto::CHECK_STATUS_UP:
      return CheckStatus::kUp;
    case proto::CHECK_STATUS_DOWN:
      return CheckStatus::kDown;
    case proto::CHECK_STATUS_UNSPECIFIED:
    case proto::CheckStatus_INT_MIN_SENTINEL_DO_NOT_USE_:
    case proto::CheckStatus_INT_MAX_SENTINEL_DO_NOT_USE_:
      break;
  }

  throw std::invalid_argument("check status must be up or down");
}

TargetSnapshot ToDomainTarget(const proto::TargetSnapshot& source) {
  return TargetSnapshot{
      .id = source.id(),
      .user_id = source.has_user_id() ? std::make_optional(source.user_id())
                                      : std::nullopt,
      .name = source.name(),
      .type = ToDomainTargetType(source.type()),
      .url = source.has_url() ? std::make_optional(source.url()) : std::nullopt,
      .method = source.has_method() ? std::make_optional(source.method())
                                    : std::nullopt,
      .expected_status_code =
          source.has_expected_status_code()
              ? std::make_optional(source.expected_status_code())
              : std::nullopt,
      .host =
          source.has_host() ? std::make_optional(source.host()) : std::nullopt,
      .port =
          source.has_port() ? std::make_optional(source.port()) : std::nullopt,
      .interval_seconds = source.interval_seconds(),
      .timeout_ms = source.timeout_ms(),
      .is_active = source.is_active(),
  };
}

CheckResultSnapshot ToDomainCheck(const proto::CheckResultSnapshot& source) {
  return CheckResultSnapshot{
      .id = source.id(),
      .target_id = source.target_id(),
      .status = ToDomainCheckStatus(source.status()),
      .protocol = ToDomainTargetType(source.protocol()),
      .http_status = source.has_http_status()
                         ? std::make_optional(source.http_status())
                         : std::nullopt,
      .latency_ms = source.has_latency_ms()
                        ? std::make_optional(source.latency_ms())
                        : std::nullopt,
      .error_message = source.has_error_message()
                           ? std::make_optional(source.error_message())
                           : std::nullopt,
      .checked_at = source.checked_at(),
  };
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
              .GetCluster()),
      alert_service_(repository_) {}

AlertGrpcService::ListAlertsResult AlertGrpcService::ListAlerts(
    CallContext&, proto::ListAlertsRequest&&) {
  return MakeAlertsResponse(repository_.ListAlerts());
}

AlertGrpcService::ListActiveAlertsResult AlertGrpcService::ListActiveAlerts(
    CallContext&, proto::ListAlertsRequest&&) {
  return MakeAlertsResponse(repository_.ListActiveAlerts());
}

AlertGrpcService::ProcessCheckResultResult AlertGrpcService::ProcessCheckResult(
    CallContext&, proto::ProcessCheckResultRequest&& request) {
  try {
    const auto target = ToDomainTarget(request.target());
    const auto previous_check =
        request.has_previous_check()
            ? std::make_optional(ToDomainCheck(request.previous_check()))
            : std::nullopt;
    const auto current_check = ToDomainCheck(request.current_check());

    alert_service_.ProcessCheckResult(target, previous_check, current_check);
    return proto::ProcessCheckResultResponse{};
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

}  // namespace netwatch::alert_service::alerts
