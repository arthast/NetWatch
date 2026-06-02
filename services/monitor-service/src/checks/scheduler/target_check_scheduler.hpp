#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <userver/components/component_base.hpp>
#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/utils/periodic_task.hpp>

#include <checks/service/check_service.hpp>
#include <checks/storage/check_lease_repository.hpp>
#include <target_client/client/target_client.hpp>
#include <target_client/model/target.hpp>

namespace netwatch::monitor_service::checks {

class TargetCheckScheduler final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "target-check-scheduler";

  TargetCheckScheduler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  void OnAllComponentsAreStopping() override;

  static userver::yaml_config::Schema GetStaticConfigSchema();

 private:
  using Clock = std::chrono::steady_clock;

  void Tick();

  bool MarkIfDue(const netwatch::target_client::Target& target,
                 Clock::time_point now);

  void LaunchCheck(netwatch::target_client::Target target);

  const netwatch::target_client::TargetClient& target_client_;
  const CheckService& check_service_;
  CheckLeaseRepository lease_repository_;
  std::string scheduler_owner_id_;
  std::chrono::milliseconds lease_duration_;
  userver::utils::PeriodicTask periodic_task_;
  userver::engine::Mutex state_mutex_;
  std::unordered_map<std::int64_t, Clock::time_point> next_check_at_;
  userver::concurrent::BackgroundTaskStorage background_tasks_;
};

}  // namespace netwatch::monitor_service::checks
