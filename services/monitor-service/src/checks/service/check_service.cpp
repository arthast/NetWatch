#include <checks/service/check_service.hpp>

#include <mutex>
#include <userver/logging/log.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <utility>

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

CheckService::CheckService(
    const netwatch::target_client::TargetClient& target_client,
    const netwatch::alert_client::AlertClient& alert_client,
    CheckRepository check_repository, CheckRunner check_runner)
    : target_client_(target_client),
      alert_client_(alert_client),
      check_repository_(std::move(check_repository)),
      check_runner_(std::move(check_runner)),
      target_mutexes_(64, 8) {}

std::optional<CheckResult> CheckService::RunCheckForTarget(
    std::int64_t target_id) const {
  const auto target = target_client_.GetTargetById(target_id);
  if (!target) {
    return std::nullopt;
  }

  return RunCheck(*target);
}

std::optional<std::vector<CheckResult>> CheckService::ListTargetChecks(
    std::int64_t target_id) const {
  if (!target_client_.GetTargetById(target_id)) {
    return std::nullopt;
  }

  return check_repository_.ListTargetChecks(target_id);
}

std::optional<CheckResult> CheckService::GetTargetStatus(
    std::int64_t target_id) const {
  if (!target_client_.GetTargetById(target_id)) {
    return std::nullopt;
  }

  return check_repository_.GetLatestTargetStatus(target_id);
}

CheckResult CheckService::RunCheck(
    const netwatch::target_client::Target& target) const {
  auto mutex = target_mutexes_.GetMutexForKey(target.id);
  std::lock_guard lock(mutex);
  return RunCheckLocked(target);
}

std::optional<CheckResult> CheckService::TryRunCheck(
    const netwatch::target_client::Target& target) const {
  auto mutex = target_mutexes_.GetMutexForKey(target.id);
  if (!mutex.try_lock()) {
    return std::nullopt;
  }

  std::unique_lock lock(mutex, std::adopt_lock);
  return RunCheckLocked(target);
}

CheckResult CheckService::RunCheckLocked(
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
  try {
    alert_client_.ProcessCheckResult(
        ToAlertTarget(target), previous_alert_check, ToAlertCheck(saved_check));
  } catch (const userver::ugrpc::client::BaseError& ex) {
    LOG_WARNING() << "Failed to process alert lifecycle for saved check, "
                  << "target_id=" << target.id
                  << ", check_id=" << saved_check.id << ", error=" << ex.what();
  }

  return saved_check;
}

}  // namespace netwatch::monitor_service::checks
