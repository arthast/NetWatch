#include <checks/service/check_service.hpp>

#include <mutex>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/component.hpp>

namespace monitor_service::checks {

CheckServiceComponent::CheckServiceComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : ComponentBase(config, component_context),
      target_client_(component_context.FindComponent<target::TargetClient>()),
      alert_client_(component_context.FindComponent<alerts::AlertClient>()),
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
    const target::Target& target) const {
  auto mutex = target_mutexes_.GetMutexForKey(target.id);
  std::lock_guard lock(mutex);
  return RunCheckLocked(target);
}

std::optional<CheckResult> CheckServiceComponent::TryRunCheck(
    const target::Target& target) const {
  auto mutex = target_mutexes_.GetMutexForKey(target.id);
  if (!mutex.try_lock()) {
    return std::nullopt;
  }

  std::unique_lock lock(mutex, std::adopt_lock);
  return RunCheckLocked(target);
}

CheckResult CheckServiceComponent::RunCheckLocked(
    const target::Target& target) const {
  const auto previous_check =
      check_repository_.GetLatestTargetStatus(target.id);
  const auto saved_check =
      check_repository_.SaveCheckResult(check_runner_.RunCheck(target));
  alert_client_.ProcessCheckResult(target, previous_check, saved_check);
  return saved_check;
}

}  // namespace monitor_service::checks
