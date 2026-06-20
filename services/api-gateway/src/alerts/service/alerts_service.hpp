#pragma once

#include <cstdint>
#include <vector>

#include <alert_client/client/alert_client.hpp>
#include <alert_client/model/alert.hpp>
#include <target_client/client/target_client.hpp>

namespace netwatch::api_gateway::alerts {

class AlertsService final {
 public:
  AlertsService(const netwatch::alert_client::AlertClient& alert_client,
                const netwatch::target_client::TargetClient& target_client);

  std::vector<netwatch::alert_client::Alert> ListAlerts(
      std::int64_t user_id) const;

  std::vector<netwatch::alert_client::Alert> ListActiveAlerts(
      std::int64_t user_id) const;

 private:
  const netwatch::alert_client::AlertClient& alert_client_;
  const netwatch::target_client::TargetClient& target_client_;
};

}  // namespace netwatch::api_gateway::alerts
