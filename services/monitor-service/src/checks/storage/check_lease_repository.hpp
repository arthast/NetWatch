#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/postgres.hpp>

namespace netwatch::monitor_service::checks {

class CheckLeaseRepository {
 public:
  explicit CheckLeaseRepository(
      userver::storages::postgres::ClusterPtr pg_cluster);

  bool TryAcquire(std::int64_t target_id, std::string_view owner_id,
                  std::chrono::milliseconds lease_duration) const;

  void Release(std::int64_t target_id, std::string_view owner_id) const;

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace netwatch::monitor_service::checks
