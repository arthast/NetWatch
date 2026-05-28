#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <userver/components/component_base.hpp>
#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/utils/periodic_task.hpp>

#include <checks/service/check_service.hpp>
#include <targets/model/target.hpp>
#include <targets/storage/target_repository.hpp>

namespace monitor_service::checks {

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

  bool MarkIfDue(const target::Target& target, Clock::time_point now);

  void LaunchCheck(target::Target target);

  target::TargetRepository target_repository_;
  const CheckServiceComponent& check_service_;
  userver::utils::PeriodicTask periodic_task_;
  userver::engine::Mutex state_mutex_;
  std::unordered_map<std::int64_t, Clock::time_point> next_check_at_;
  userver::concurrent::BackgroundTaskStorage background_tasks_;
};

}  // namespace monitor_service::checks
