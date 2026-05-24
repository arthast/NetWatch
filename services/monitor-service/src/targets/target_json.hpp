#pragma once

#include <string_view>
#include <userver/formats/json/value.hpp>
#include <vector>

#include "target.hpp"

namespace monitor_service::target {
CreateTargetRequest ParseCreateTargetRequest(
    const userver::formats::json::Value &json);

userver::formats::json::Value SerializeTarget(const Target &target);

userver::formats::json::Value SerializeTargets(
    const std::vector<Target> &targets);

userver::formats::json::Value SerializeError(std::string_view message);
} // namespace monitor_service::target
