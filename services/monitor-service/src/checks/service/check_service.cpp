#include <checks/service/check_service.hpp>

#include <mutex>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/component.hpp>

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

CheckServiceComponent::CheckServiceComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : ComponentBase(config, component_context),
      target_client_(
          component_context
              .FindComponent<netwatch::target_client::TargetClient>()),
      alert_client_(component_context
                        .FindComponent<netwatch::alert_client::AlertClient>()),
      check_repository_(
          component_context
              .FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()),
      check_runner_(
          component_context.FindComponent<userver::components::HttpClient>()
              .GetHttpClient(),
          component_context.FindComponent<userver::clients::dns::Component>()
              .GetResolver()),
      target_mutexes_(64, 8) {}

std::optional<CheckResult> CheckServiceComponent::RunCheckForTarget(
    std::int64_t target_id) const {
  const auto target = target_client_.GetTargetById(target_id);
  if (!target) {
    return std::nullopt;
  }

  return RunCheck(*target);
}

std::optional<std::vector<CheckResult>> CheckServiceComponent::ListTargetChecks(
    std::int64_t target_id) const {
  if (!target_client_.GetTargetById(target_id)) {
    return std::nullopt;
  }

  return check_repository_.ListTargetChecks(target_id);
}

std::optional<CheckResult> CheckServiceComponent::GetTargetStatus(
    std::int64_t target_id) const {
  if (!target_client_.GetTargetById(target_id)) {
    return std::nullopt;
  }

  return check_repository_.GetLatestTargetStatus(target_id);
}

CheckResult CheckServiceComponent::RunCheck(
    const netwatch::target_client::Target& target) const {
  auto mutex = target_mutexes_.GetMutexForKey(target.id);
  std::lock_guard lock(mutex);
  return RunCheckLocked(target);
}

std::optional<CheckResult> CheckServiceComponent::TryRunCheck(
    const netwatch::target_client::Target& target) const {
  auto mutex = target_mutexes_.GetMutexForKey(target.id);
  if (!mutex.try_lock()) {
    return std::nullopt;
  }

  std::unique_lock lock(mutex, std::adopt_lock);
  return RunCheckLocked(target);
}

CheckResult CheckServiceComponent::RunCheckLocked(
    const netwatch::target_client::Target& target) const {
  const auto previous_check =
      check_repository_.GetLatestTargetStatus(target.id);
  const auto saved_check =
      check_repository_.SaveCheckResult(check_runner_.RunCheck(target));

  std::optional<netwatch::alert_client::CheckResultSnapshot>
      previous_alert_check;
  if (previous_check) {
    previous_alert_check = ToAlertCheck(*previous_check);
  }
  alert_client_.ProcessCheckResult(ToAlertTarget(target), previous_alert_check,
                                   ToAlertCheck(saved_check));
  return saved_check;
}

}  // namespace netwatch::monitor_service::checks
