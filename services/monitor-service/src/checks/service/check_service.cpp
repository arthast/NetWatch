#include <checks/service/check_service.hpp>

#include <mutex>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/component.hpp>

#include <alerts/storage/alert_repository.hpp>

namespace monitor_service::checks {

CheckServiceComponent::CheckServiceComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : ComponentBase(config, component_context),
      target_repository_(
          component_context
              .FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()),
      check_repository_(
          component_context
              .FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()),
      check_runner_(
          component_context.FindComponent<userver::components::HttpClient>()
              .GetHttpClient(),
          component_context.FindComponent<userver::clients::dns::Component>()
              .GetResolver()),
      alert_service_(alerts::AlertRepository{
          component_context
              .FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()}),
      target_mutexes_(64, 8) {}

std::optional<CheckResult> CheckServiceComponent::RunCheckForTarget(
    std::int64_t target_id) const {
  const auto target = target_repository_.GetTargetById(target_id);
  if (!target) {
    return std::nullopt;
  }

  return RunCheck(*target);
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
  alert_service_.ProcessCheckResult(target, previous_check, saved_check);
  return saved_check;
}

}  // namespace monitor_service::checks
