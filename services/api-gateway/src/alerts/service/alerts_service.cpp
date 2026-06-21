#include <alerts/service/alerts_service.hpp>

#include <unordered_set>
#include <utility>

namespace netwatch::api_gateway::alerts {
namespace {

std::vector<netwatch::alert_client::Alert> FilterAlertsForUserTargets(
    std::vector<netwatch::alert_client::Alert> alerts,
    const std::vector<netwatch::target_client::Target>& targets) {
  std::unordered_set<std::int64_t> target_ids;
  target_ids.reserve(targets.size());
  for (const auto& target : targets) {
    target_ids.insert(target.id);
  }

  std::vector<netwatch::alert_client::Alert> filtered;
  filtered.reserve(alerts.size());
  for (auto& alert : alerts) {
    if (target_ids.count(alert.target_id) != 0) {
      filtered.push_back(std::move(alert));
    }
  }
  return filtered;
}

}  // namespace

AlertsService::AlertsService(
    const netwatch::alert_client::AlertClient& alert_client,
    const netwatch::target_client::TargetClient& target_client)
    : alert_client_(alert_client), target_client_(target_client) {}

std::vector<netwatch::alert_client::Alert> AlertsService::ListAlerts(
    std::int64_t user_id) const {
  return FilterAlertsForUserTargets(
      alert_client_.ListAlerts(),
      target_client_.ListActiveTargetsForUser(user_id));
}

std::vector<netwatch::alert_client::Alert> AlertsService::ListActiveAlerts(
    std::int64_t user_id) const {
  return FilterAlertsForUserTargets(
      alert_client_.ListActiveAlerts(),
      target_client_.ListActiveTargetsForUser(user_id));
}

}  // namespace netwatch::api_gateway::alerts
