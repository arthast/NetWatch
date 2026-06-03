#include <checks/alerting/check_alert_notifier.hpp>

#include <userver/logging/log.hpp>
#include <userver/ugrpc/client/exceptions.hpp>

namespace netwatch::monitor_service::checks {
namespace {

netwatch::alert_client::TargetType ToAlertTargetType(
    netwatch::target_client::TargetType type) {
  switch (type) {
    case netwatch::target_client::TargetType::kHttp:
      return netwatch::alert_client::TargetType::kHttp;
    case netwatch::target_client::TargetType::kTcp:
      return netwatch::alert_client::TargetType::kTcp;
  }

  return netwatch::alert_client::TargetType::kHttp;
}

netwatch::alert_client::CheckStatus ToAlertCheckStatus(CheckStatus status) {
  switch (status) {
    case CheckStatus::kUp:
      return netwatch::alert_client::CheckStatus::kUp;
    case CheckStatus::kDown:
      return netwatch::alert_client::CheckStatus::kDown;
  }

  return netwatch::alert_client::CheckStatus::kDown;
}

netwatch::alert_client::TargetType ToAlertCheckProtocol(
    CheckProtocol protocol) {
  switch (protocol) {
    case CheckProtocol::kHttp:
      return netwatch::alert_client::TargetType::kHttp;
    case CheckProtocol::kTcp:
      return netwatch::alert_client::TargetType::kTcp;
  }
  return netwatch::alert_client::TargetType::kHttp;
}

netwatch::alert_client::TargetSnapshot ToAlertTarget(
    const netwatch::target_client::Target& source) {
  return netwatch::alert_client::TargetSnapshot{
      .id = source.id,
      .name = source.name,
      .type = ToAlertTargetType(source.type),
      .url = source.url,
      .method = source.method,
      .expected_status_code = source.expected_status_code,
      .host = source.host,
      .port = source.port,
      .interval_seconds = source.interval_seconds,
      .timeout_ms = source.timeout_ms,
      .is_active = source.is_active,
  };
}

netwatch::alert_client::CheckResultSnapshot ToAlertCheck(
    const CheckResult& source) {
  return netwatch::alert_client::CheckResultSnapshot{
      .id = source.id,
      .target_id = source.target_id,
      .status = ToAlertCheckStatus(source.status),
      .protocol = ToAlertCheckProtocol(source.protocol),
      .http_status = source.http_status,
      .latency_ms = source.latency_ms,
      .error_message = source.error_message,
      .checked_at = source.checked_at,
  };
}

}  // namespace

CheckAlertNotifier::CheckAlertNotifier(
    const netwatch::alert_client::AlertClient& alert_client)
    : alert_client_(alert_client) {}

void CheckAlertNotifier::ProcessCheckResult(
    const netwatch::target_client::Target& target,
    const std::optional<CheckResult>& previous_check,
    const CheckResult& current_check) const {
  std::optional<netwatch::alert_client::CheckResultSnapshot>
      previous_alert_check;
  if (previous_check) {
    previous_alert_check = ToAlertCheck(*previous_check);
  }

  try {
    alert_client_.ProcessCheckResult(ToAlertTarget(target),
                                     previous_alert_check,
                                     ToAlertCheck(current_check));
  } catch (const userver::ugrpc::client::BaseError& ex) {
    LOG_WARNING() << "Failed to process alert lifecycle for saved check, "
                  << "target_id=" << target.id
                  << ", check_id=" << current_check.id
                  << ", error=" << ex.what();
  }
}

}  // namespace netwatch::monitor_service::checks
