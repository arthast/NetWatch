#pragma once

#include <string_view>
#include <userver/formats/json/value.hpp>
#include <vector>

#include <targets/model/target.hpp>

namespace netwatch::api_gateway::targets {
netwatch::target_client::CreateTargetRequest ParseCreateTargetRequest(
    const userver::formats::json::Value& json);

netwatch::target_client::UpdateTargetRequest ParseUpdateTargetRequest(
    const userver::formats::json::Value& json);

userver::formats::json::Value SerializeTarget(
    const netwatch::target_client::Target& target);

userver::formats::json::Value SerializeTargets(
    const std::vector<netwatch::target_client::Target>& targets);

userver::formats::json::Value SerializeError(std::string_view message);
}  // namespace netwatch::api_gateway::targets
