#pragma once

#include <vector>

#include <alert_client/client/alert_client.hpp>
#include <alert_client/model/alert.hpp>

namespace netwatch::api_gateway::alerts {

class AlertsService final {
 public:
  explicit AlertsService(
      const netwatch::alert_client::AlertClient& alert_client);

  std::vector<netwatch::alert_client::Alert> ListAlerts() const;

  std::vector<netwatch::alert_client::Alert> ListActiveAlerts() const;

 private:
  const netwatch::alert_client::AlertClient& alert_client_;
};

}  // namespace netwatch::api_gateway::alerts
