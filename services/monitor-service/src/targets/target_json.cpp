#include "target_json.hpp"

#include <userver/formats/json/value_builder.hpp>

#include <optional>
#include <stdexcept>
#include <string>

namespace monitor_service::target {

namespace {

template <typename T>
T ReadRequired(const userver::formats::json::Value& json, std::string_view field) {
    const auto value = json[field];
    if (value.IsMissing() || value.IsNull()) {
        throw std::invalid_argument("field '" + std::string{field} + "' is required");
    }
    return value.As<T>();
}

template <typename T>
std::optional<T> ReadOptional(const userver::formats::json::Value& json, std::string_view field) {
    const auto value = json[field];
    if (value.IsMissing() || value.IsNull()) {
        return std::nullopt;
    }
    return value.As<T>();
}

template <typename T>
void SetOptional(userver::formats::json::ValueBuilder& builder, std::string_view field, const std::optional<T>& value) {
    if (value) {
        builder[std::string{field}] = *value;
    }
}

}  // namespace

CreateTargetRequest ParseCreateTargetRequest(const userver::formats::json::Value& json) {
    if (!json.IsObject()) {
        throw std::invalid_argument("request body must be a JSON object");
    }

    auto expected_status_code = ReadOptional<int>(json, "expected_status_code");
    if (!expected_status_code) {
        expected_status_code = ReadOptional<int>(json, "expected_status");
    }

    auto request = CreateTargetRequest{
        .name = ReadRequired<std::string>(json, "name"),
        .type = TargetTypeFromString(ReadRequired<std::string>(json, "type")),
        .url = ReadOptional<std::string>(json, "url"),
        .method = ReadOptional<std::string>(json, "method"),
        .expected_status_code = expected_status_code,
        .host = ReadOptional<std::string>(json, "host"),
        .port = ReadOptional<int>(json, "port"),
        .interval_seconds = ReadRequired<int>(json, "interval_seconds"),
        .timeout_ms = ReadRequired<int>(json, "timeout_ms"),
    };

    if (request.type == TargetType::kHttp) {
        if (!request.method) {
            request.method = "GET";
        }
        if (!request.expected_status_code) {
            request.expected_status_code = 200;
        }
    }

    return request;
}

userver::formats::json::Value SerializeTarget(const Target& target) {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = target.id;
    builder["name"] = target.name;
    builder["type"] = TargetTypeToString(target.type);
    SetOptional(builder, "url", target.url);
    SetOptional(builder, "method", target.method);
    SetOptional(builder, "expected_status_code", target.expected_status_code);
    SetOptional(builder, "host", target.host);
    SetOptional(builder, "port", target.port);
    builder["interval_seconds"] = target.interval_seconds;
    builder["timeout_ms"] = target.timeout_ms;
    builder["is_active"] = target.is_active;
    return builder.ExtractValue();
}

userver::formats::json::Value SerializeError(std::string_view message) {
    userver::formats::json::ValueBuilder builder;
    builder["error"] = std::string{message};
    return builder.ExtractValue();
}

}  // namespace monitor_service::target
