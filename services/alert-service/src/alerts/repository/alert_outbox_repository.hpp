#pragma once

#include <chrono>
#include <string>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/postgres.hpp>
#include <vector>

#include <alerts/events/alert_event.hpp>

namespace netwatch::alert_service::alerts {

class AlertOutboxRepository {
 public:
  explicit AlertOutboxRepository(
      userver::storages::postgres::ClusterPtr pg_cluster);

  std::vector<AlertOutboxEvent> AcquirePendingEvents(int limit) const;

  void MarkPublished(const std::string& event_id) const;

  void MarkFailed(const std::string& event_id, const std::string& last_error,
                  std::chrono::milliseconds retry_delay) const;

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace netwatch::alert_service::alerts
