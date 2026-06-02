#pragma once

#include <cstdint>
#include <optional>
#include <userver/concurrent/mutex_set.hpp>
#include <vector>

#include <alert_client/client/alert_client.hpp>
#include <checks/model/check_result.hpp>
#include <checks/runner/check_runner.hpp>
#include <checks/storage/check_repository.hpp>
#include <target_client/client/target_client.hpp>
#include <target_client/model/target.hpp>

namespace netwatch::monitor_service::checks {

class CheckService final {
 public:
  CheckService(const netwatch::target_client::TargetClient& target_client,
               const netwatch::alert_client::AlertClient& alert_client,
               CheckRepository check_repository, CheckRunner check_runner);

  std::optional<CheckResult> RunCheckForTarget(std::int64_t target_id) const;

  std::optional<std::vector<CheckResult>> ListTargetChecks(
      std::int64_t target_id) const;

  std::optional<CheckResult> GetTargetStatus(std::int64_t target_id) const;

  CheckResult RunCheck(const netwatch::target_client::Target& target) const;

  std::optional<CheckResult> TryRunCheck(
      const netwatch::target_client::Target& target) const;

 private:
  CheckResult RunCheckLocked(
      const netwatch::target_client::Target& target) const;

  const netwatch::target_client::TargetClient& target_client_;
  const netwatch::alert_client::AlertClient& alert_client_;
  CheckRepository check_repository_;
  CheckRunner check_runner_;
  mutable userver::concurrent::MutexSet<std::int64_t> target_mutexes_;
};

}  // namespace netwatch::monitor_service::checks
