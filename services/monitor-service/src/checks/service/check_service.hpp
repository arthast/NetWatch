#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component_base.hpp>
#include <userver/concurrent/mutex_set.hpp>
#include <vector>

#include <alerts/service/alert_service.hpp>
#include <checks/model/check_result.hpp>
#include <checks/runner/check_runner.hpp>
#include <checks/storage/check_repository.hpp>
#include <targets/client/target_client.hpp>
#include <targets/model/target.hpp>

namespace monitor_service::checks {

class CheckServiceComponent final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "check-service";

  CheckServiceComponent(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  std::optional<CheckResult> RunCheckForTarget(std::int64_t target_id) const;

  std::optional<std::vector<CheckResult>> ListTargetChecks(
      std::int64_t target_id) const;

  std::optional<CheckResult> GetTargetStatus(std::int64_t target_id) const;

  CheckResult RunCheck(const target::Target& target) const;

  std::optional<CheckResult> TryRunCheck(const target::Target& target) const;

 private:
  CheckResult RunCheckLocked(const target::Target& target) const;

  const target::TargetClient& target_client_;
  CheckRepository check_repository_;
  CheckRunner check_runner_;
  alerts::AlertService alert_service_;
  mutable userver::concurrent::MutexSet<std::int64_t> target_mutexes_;
};

}  // namespace monitor_service::checks
