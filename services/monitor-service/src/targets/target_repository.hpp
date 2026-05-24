#pragma once

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/postgres.hpp>
#include <vector>

#include "target.hpp"

namespace monitor_service::target {

class TargetRepository {
 public:
  explicit TargetRepository(userver::storages::postgres::ClusterPtr pg_cluster);

  Target CreateTarget(const CreateTargetRequest& request) const;
  std::vector<Target> ListActiveTargets() const;

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace monitor_service::target
