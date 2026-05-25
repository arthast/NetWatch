#include <targets/validation/target_validator.hpp>

#include <optional>
#include <string_view>

namespace monitor_service::target_validator {
namespace {
    bool IsBlank(std::string_view value) {
        return value.find_first_not_of(" \t\n\r") == std::string_view::npos;
    }

    bool IsValidPort(int port) {
        return port >= 1 && port <= 65535;
    }
} // namespace

std::optional<std::string> ValidateCreateTargetRequest(
    const target::CreateTargetRequest &request
) {
    if (IsBlank(request.name)) {
        return "name must not be empty";
    }

    if (request.interval_seconds < 5) {
        return "interval_seconds must be at least 5";
    }

    if (request.timeout_ms <= 0) {
        return "timeout_ms must be greater than zero";
    }

    const auto interval_ms = static_cast<std::int64_t>(request.interval_seconds) * 1000;
    if (request.timeout_ms >= interval_ms) {
        return "timeout_ms must be less than interval_seconds in milliseconds";
    }

    switch (request.type) {
        case target::TargetType::kHttp:
            if (!request.url || IsBlank(*request.url)) {
                return "http target requires url";
            }
            if (request.host) {
                return "http target must not contain host";
            }
            if (request.port) {
                return "http target must not contain port";
            }
            if (request.method && IsBlank(*request.method)) {
                return "http target method must not be empty";
            }
            if (request.expected_status_code &&
                (*request.expected_status_code < 100 || *request.expected_status_code > 599)) {
                return "expected_status_code must be a valid HTTP status code";
            }
            break;

        case target::TargetType::kTcp:
            if (!request.host || IsBlank(*request.host)) {
                return "tcp target requires host";
            }
            if (!request.port) {
                return "tcp target requires port";
            }
            if (!IsValidPort(*request.port)) {
                return "tcp target port must be between 1 and 65535";
            }
            if (request.url) {
                return "tcp target must not contain url";
            }
            if (request.method) {
                return "tcp target must not contain method";
            }
            if (request.expected_status_code) {
                return "tcp target must not contain expected_status_code";
            }
    }

    return std::nullopt;
}

std::optional<std::string> ValidateTarget(const target::Target &target) {
    return ValidateCreateTargetRequest(target::CreateTargetRequest{
        .name = target.name,
        .type = target.type,
        .url = target.url,
        .method = target.method,
        .expected_status_code = target.expected_status_code,
        .host = target.host,
        .port = target.port,
        .interval_seconds = target.interval_seconds,
        .timeout_ms = target.timeout_ms,
    });
}
} // namespace monitor_service::target_validator
