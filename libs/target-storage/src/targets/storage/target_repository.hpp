#pragma once

#include <optional>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/postgres.hpp>
#include <vector>

#include <targets/model/target.hpp>

namespace monitor_service::target {
class TargetRepository {
 public:
  explicit TargetRepository(userver::storages::postgres::ClusterPtr pg_cluster);

  Target CreateTarget(const CreateTargetRequest& request) const;

  std::vector<Target> ListTargets() const;

  std::vector<Target> ListActiveTargets() const;

  std::optional<Target> GetTargetById(std::int64_t target_id) const;

  std::optional<Target> UpdateTarget(const Target& target) const;

  bool DeactivateTarget(std::int64_t target_id) const;

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};
}  // namespace monitor_service::target
