#include <checks/scheduler/target_check_scheduler.hpp>

#include <chrono>
#include <cstdint>
#include <exception>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

namespace netwatch::monitor_service::checks {
namespace {
constexpr auto kDefaultScanPeriod = std::chrono::milliseconds{1000};
constexpr auto kDefaultLeaseDuration = std::chrono::milliseconds{30000};

std::string MakeSchedulerOwnerId() {
  std::random_device random;
  const auto now =
      std::chrono::system_clock::now().time_since_epoch().count();

  std::ostringstream stream;
  stream << "monitor-service-scheduler-" << now << '-' << random();
  return stream.str();
}
}  // namespace

TargetCheckScheduler::TargetCheckScheduler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : ComponentBase(config, component_context),
      target_client_(
          component_context
              .FindComponent<netwatch::target_client::TargetClient>()),
      check_service_(component_context.FindComponent<CheckServiceComponent>()),
      lease_repository_(
          component_context
              .FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()),
      scheduler_owner_id_(MakeSchedulerOwnerId()),
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
  lease_duration_ =
      std::chrono::milliseconds{config["lease-duration-ms"].As<int>(
          static_cast<int>(kDefaultLeaseDuration.count()))};
  if (lease_duration_.count() <= 0) {
    throw std::invalid_argument{
        "target-check-scheduler.lease-duration-ms must be positive"};
  }

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
    lease-duration-ms:
        type: integer
        description: distributed per-target check lease duration in milliseconds
        defaultDescription: 30000
)");
}

void TargetCheckScheduler::Tick() {
  std::vector<netwatch::target_client::Target> targets;
  try {
    targets = target_client_.ListActiveTargets();
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Target check scheduler skipped tick, failed to list "
                  << "active targets: " << ex.what();
    return;
  }

  const auto now = Clock::now();
  for (const auto& target : targets) {
    if (MarkIfDue(target, now)) {
      LaunchCheck(target);
    }
  }
}

bool TargetCheckScheduler::MarkIfDue(
    const netwatch::target_client::Target& target, Clock::time_point now) {
  std::lock_guard lock(state_mutex_);

  const auto next_check_it = next_check_at_.find(target.id);
  if (next_check_it != next_check_at_.end() && now < next_check_it->second) {
    return false;
  }

  next_check_at_[target.id] =
      now + std::chrono::seconds{target.interval_seconds};
  return true;
}

void TargetCheckScheduler::LaunchCheck(netwatch::target_client::Target target) {
  background_tasks_.AsyncDetach(
      "target-check-" + std::to_string(target.id),
      [this, target = std::move(target)] {
        try {
          if (!lease_repository_.TryAcquire(target.id, scheduler_owner_id_,
                                            lease_duration_)) {
            LOG_DEBUG() << "Skipped scheduled check because target lease is "
                           "held by another scheduler, target_id="
                        << target.id;
            return;
          }

          const auto release_lease = [this, target_id = target.id] {
            try {
              lease_repository_.Release(target_id, scheduler_owner_id_);
            } catch (const std::exception& ex) {
              LOG_WARNING() << "Failed to release target check lease, "
                            << "target_id=" << target_id
                            << ", owner_id=" << scheduler_owner_id_
                            << ", error=" << ex.what();
            }
          };

          const auto check = check_service_.TryRunCheck(target);
          if (!check) {
            LOG_DEBUG() << "Skipped scheduled check because target is already "
                           "running, target_id="
                        << target.id;
            release_lease();
            return;
          }

          LOG_INFO() << "Scheduled check finished, target_id=" << target.id
                     << ", check_id=" << check->id
                     << ", status=" << CheckStatusToString(check->status);
          release_lease();
        } catch (const std::exception& ex) {
          LOG_ERROR() << "Scheduled check failed, target_id=" << target.id
                      << ", error=" << ex.what();
          try {
            lease_repository_.Release(target.id, scheduler_owner_id_);
          } catch (const std::exception& release_ex) {
            LOG_WARNING() << "Failed to release target check lease after "
                          << "scheduled check error, target_id=" << target.id
                          << ", owner_id=" << scheduler_owner_id_
                          << ", error=" << release_ex.what();
          }
        }
      });
}
}  // namespace netwatch::monitor_service::checks
