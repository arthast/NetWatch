#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace monitor_service::target {
enum class TargetType { kHttp, kTcp };

struct Target {
    std::int64_t id;
    std::string name;
    TargetType type;

    std::optional<std::string> url;
    std::optional<std::string> method;
    std::optional<int> expected_status_code;

    std::optional<std::string> host;
    std::optional<int> port;

    int interval_seconds;
    int timeout_ms;
    bool is_active;
};

struct CreateTargetRequest {
    std::string name;
    TargetType type;

    std::optional<std::string> url;
    std::optional<std::string> method;
    std::optional<int> expected_status_code;

    std::optional<std::string> host;
    std::optional<int> port;

    int interval_seconds;
    int timeout_ms;
};

TargetType TargetTypeFromString(const std::string &value);

std::string TargetTypeToString(TargetType type);
} // namespace monitor_service::target
