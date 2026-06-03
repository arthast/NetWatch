#include <checks/service/check_service.hpp>

#include <mutex>
#include <utility>

namespace netwatch::monitor_service::checks {

CheckService::CheckService(
    const netwatch::target_client::TargetClient& target_client,
    CheckAlertNotifier alert_notifier, CheckRepository check_repository,
    CheckRunner check_runner)
    : target_client_(target_client),
      alert_notifier_(std::move(alert_notifier)),
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

  alert_notifier_.ProcessCheckResult(target, previous_check, saved_check);

  return saved_check;
}

}  // namespace netwatch::monitor_service::checks
