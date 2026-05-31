#pragma once

#include <cstdint>
#include <optional>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/postgres.hpp>
#include <vector>

#include <alerts/model/alert.hpp>

namespace netwatch::alert_service::alerts {

class AlertRepository {
 public:
  explicit AlertRepository(userver::storages::postgres::ClusterPtr pg_cluster);

  Alert CreateAlert(const NewAlert& alert) const;

  std::optional<Alert> FindActiveAlert(std::int64_t target_id,
                                       AlertType type) const;

  std::optional<Alert> ResolveActiveAlert(std::int64_t target_id,
                                          AlertType type) const;

  std::vector<Alert> ListAlerts() const;

  std::vector<Alert> ListActiveAlerts() const;

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace netwatch::alert_service::alerts
