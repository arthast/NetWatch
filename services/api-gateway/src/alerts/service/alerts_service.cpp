#include <alerts/service/alerts_service.hpp>

namespace netwatch::api_gateway::alerts {

AlertsService::AlertsService(
    const netwatch::alert_client::AlertClient& alert_client)
    : alert_client_(alert_client) {}

std::vector<netwatch::alert_client::Alert> AlertsService::ListAlerts() const {
  return alert_client_.ListAlerts();
}

std::vector<netwatch::alert_client::Alert> AlertsService::ListActiveAlerts()
    const {
  return alert_client_.ListActiveAlerts();
}

}  // namespace netwatch::api_gateway::alerts
