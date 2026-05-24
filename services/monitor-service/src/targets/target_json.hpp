#pragma once

#include "target.hpp"

#include <userver/formats/json/value.hpp>

#include <string_view>

namespace monitor_service::target {

CreateTargetRequest ParseCreateTargetRequest(const userver::formats::json::Value& json);

userver::formats::json::Value SerializeTarget(const Target& target);

userver::formats::json::Value SerializeError(std::string_view message);

}  // namespace monitor_service::target
