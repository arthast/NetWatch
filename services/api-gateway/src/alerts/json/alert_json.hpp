#pragma once

#include <userver/formats/json/value.hpp>
#include <vector>

#include <alert_client/model/alert.hpp>

namespace netwatch::api_gateway::alerts {

userver::formats::json::Value SerializeAlert(
    const netwatch::alert_client::Alert& alert);

userver::formats::json::Value SerializeAlerts(
    const std::vector<netwatch::alert_client::Alert>& alerts);

}  // namespace netwatch::api_gateway::alerts
