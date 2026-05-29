#pragma once

#include <cstdint>
#include <optional>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/postgres.hpp>
#include <vector>

#include <checks/model/check_result.hpp>

namespace monitor_service::checks {

class CheckRepository {
 public:
  explicit CheckRepository(userver::storages::postgres::ClusterPtr pg_cluster);

  CheckResult SaveCheckResult(const CheckResult& check) const;

  std::vector<CheckResult> ListTargetChecks(std::int64_t target_id) const;

  std::optional<CheckResult> GetLatestTargetStatus(
      std::int64_t target_id) const;

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace monitor_service::checks
