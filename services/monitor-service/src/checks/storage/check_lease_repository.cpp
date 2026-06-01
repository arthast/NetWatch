#include <checks/storage/check_lease_repository.hpp>

#include <utility>
#include <userver/storages/postgres/result_set.hpp>

namespace netwatch::monitor_service::checks {

CheckLeaseRepository::CheckLeaseRepository(
    userver::storages::postgres::ClusterPtr pg_cluster)
    : pg_cluster_(std::move(pg_cluster)) {}

bool CheckLeaseRepository::TryAcquire(
    std::int64_t target_id, std::string_view owner_id,
    std::chrono::milliseconds lease_duration) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            INSERT INTO target_check_leases (
                target_id,
                owner_id,
                locked_until,
                updated_at
            )
            VALUES (
                $1,
                $2,
                NOW() + ($3::bigint * INTERVAL '1 millisecond'),
                NOW()
            )
            ON CONFLICT (target_id) DO UPDATE
            SET owner_id = EXCLUDED.owner_id,
                locked_until = EXCLUDED.locked_until,
                updated_at = NOW()
            WHERE target_check_leases.locked_until <= NOW()
               OR target_check_leases.owner_id = EXCLUDED.owner_id
            RETURNING target_id
        )",
      target_id, std::string{owner_id}, lease_duration.count());

  return result.Size() != 0;
}

void CheckLeaseRepository::Release(std::int64_t target_id,
                                   std::string_view owner_id) const {
  pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            DELETE FROM target_check_leases
            WHERE target_id = $1
              AND owner_id = $2
        )",
      target_id, std::string{owner_id});
}

}  // namespace netwatch::monitor_service::checks
