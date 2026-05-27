#include <checks/scheduler/target_check_scheduler.hpp>

#include <chrono>
#include <exception>
#include <string>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

namespace monitor_service::checks {
namespace {
constexpr auto kDefaultScanPeriod = std::chrono::milliseconds{1000};
}  // namespace

TargetCheckScheduler::TargetCheckScheduler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : ComponentBase(config, component_context),
      target_repository_(
          component_context
              .FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()),
      check_service_(component_context.FindComponent<CheckServiceComponent>()),
      background_tasks_(
          component_context.GetTaskProcessor("main-task-processor")) {
  const auto enabled = config["enabled"].As<bool>(true);
  if (!enabled) {
    LOG_INFO() << "Target check scheduler is disabled";
    return;
  }

  const auto scan_period =
      std::chrono::milliseconds{config["scan-period-ms"].As<int>(
          static_cast<int>(kDefaultScanPeriod.count()))};

  auto& testsuite_tasks =
      component_context.FindComponent<userver::components::TestsuiteSupport>()
          .GetTestsuiteTasks();

  userver::utils::StartPeriodicTask(
      periodic_task_, std::string{kName},
      userver::utils::PeriodicTask::Settings{
          scan_period, {userver::utils::PeriodicTask::Flags::kStrong}},
      [this] { Tick(); }, testsuite_tasks);
}

void TargetCheckScheduler::OnAllComponentsAreStopping() {
  periodic_task_.Stop();
  background_tasks_.CancelAndWait();
}

userver::yaml_config::Schema TargetCheckScheduler::GetStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(
      R"(
type: object
description: periodically runs checks for active targets
additionalProperties: false
properties:
    enabled:
        type: boolean
        description: enables periodic target checks
        defaultDescription: true
    scan-period-ms:
        type: integer
        description: scheduler loop period in milliseconds
        defaultDescription: 1000
)");
}

void TargetCheckScheduler::Tick() {
  const auto targets = target_repository_.ListActiveTargets();
  const auto now = Clock::now();

  for (const auto& target : targets) {
    if (MarkIfDue(target, now)) {
      LaunchCheck(target);
    }
  }
}

bool TargetCheckScheduler::MarkIfDue(const target::Target& target,
                                     Clock::time_point now) {
  std::lock_guard lock(state_mutex_);

  const auto next_check_it = next_check_at_.find(target.id);
  if (next_check_it != next_check_at_.end() && now < next_check_it->second) {
    return false;
  }

  next_check_at_[target.id] =
      now + std::chrono::seconds{target.interval_seconds};
  return true;
}

void TargetCheckScheduler::LaunchCheck(target::Target target) {
  background_tasks_.AsyncDetach(
      "target-check-" + std::to_string(target.id),
      [this, target = std::move(target)] {
        try {
          const auto check = check_service_.TryRunCheck(target);
          if (!check) {
            LOG_DEBUG() << "Skipped scheduled check because target is already "
                           "running, target_id="
                        << target.id;
            return;
          }

          LOG_INFO() << "Scheduled check finished, target_id=" << target.id
                     << ", check_id=" << check->id
                     << ", status=" << CheckStatusToString(check->status);
        } catch (const std::exception& ex) {
          LOG_ERROR() << "Scheduled check failed, target_id=" << target.id
                      << ", error=" << ex.what();
        }
      });
}
}  // namespace monitor_service::checks
