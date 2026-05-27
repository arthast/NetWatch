#pragma once

#include <userver/formats/json/value.hpp>
#include <vector>

#include <checks/model/check_result.hpp>

namespace monitor_service::checks {

userver::formats::json::Value SerializeCheckResult(const CheckResult& check);

userver::formats::json::Value SerializeCheckResults(
    const std::vector<CheckResult>& checks);

}  // namespace monitor_service::checks
