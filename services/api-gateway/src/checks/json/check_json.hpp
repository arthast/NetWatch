#pragma once

#include <userver/formats/json/value.hpp>
#include <vector>

#include <monitor_client/model/check_result.hpp>

namespace netwatch::api_gateway::checks {

userver::formats::json::Value SerializeCheckResult(
    const netwatch::monitor_client::CheckResult& check);

userver::formats::json::Value SerializeCheckResults(
    const std::vector<netwatch::monitor_client::CheckResult>& checks);

}  // namespace netwatch::api_gateway::checks
