#pragma once

#include <userver/storages/postgres/postgres.hpp>
#include <userver/storages/postgres/cluster.hpp>

#include "target.hpp"

namespace monitor_service::target {

class TargetRepository {
public:
    explicit TargetRepository(userver::storages::postgres::ClusterPtr pg_cluster);

    Target CreateTarget(const CreateTargetRequest& request) const;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace monitor_service::target
