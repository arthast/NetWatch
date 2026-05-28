#pragma once

#include <userver/formats/json/value.hpp>
#include <vector>

#include <alerts/model/alert.hpp>

namespace monitor_service::alerts {

userver::formats::json::Value SerializeAlert(const Alert& alert);

userver::formats::json::Value SerializeAlerts(const std::vector<Alert>& alerts);

}  // namespace monitor_service::alerts
