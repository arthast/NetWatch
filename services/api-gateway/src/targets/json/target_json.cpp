#include <targets/json/target_json.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <vector>

#include <common/json.hpp>

namespace netwatch::api_gateway::targets {
namespace target_client = netwatch::target_client;
namespace {
template <typename T>
T ReadRequired(const userver::formats::json::Value& json,
               std::string_view field) {
  const auto value = json[field];
  if (value.IsMissing() || value.IsNull()) {
    throw std::invalid_argument("field '" + std::string{field} +
                                "' is required");
  }
  return value.As<T>();
}

template <typename T>
std::optional<T> ReadOptional(const userver::formats::json::Value& json,
                              std::string_view field) {
  const auto value = json[field];
  if (value.IsMissing() || value.IsNull()) {
    return std::nullopt;
  }
  return value.As<T>();
}

template <typename T>
std::optional<T> ReadPatchOptional(const userver::formats::json::Value& json,
                                   std::string_view field) {
  const auto value = json[field];
  if (value.IsMissing()) {
    return std::nullopt;
  }
  if (value.IsNull()) {
    throw std::invalid_argument("field '" + std::string{field} +
                                "' must not be null");
  }
  return value.As<T>();
}

}  // namespace

target_client::CreateTargetRequest ParseCreateTargetRequest(
    const userver::formats::json::Value& json) {
  if (!json.IsObject()) {
    throw std::invalid_argument("request body must be a JSON object");
  }

  auto expected_status_code = ReadOptional<int>(json, "expected_status_code");
  if (!expected_status_code) {
    expected_status_code = ReadOptional<int>(json, "expected_status");
  }

  auto request = target_client::CreateTargetRequest{
      .name = ReadRequired<std::string>(json, "name"),
      .type = target_client::TargetTypeFromString(
          ReadRequired<std::string>(json, "type")),
      .url = ReadOptional<std::string>(json, "url"),
      .method = ReadOptional<std::string>(json, "method"),
      .expected_status_code = expected_status_code,
      .host = ReadOptional<std::string>(json, "host"),
      .port = ReadOptional<int>(json, "port"),
      .interval_seconds = ReadRequired<int>(json, "interval_seconds"),
      .timeout_ms = ReadRequired<int>(json, "timeout_ms"),
  };

  if (request.type == target_client::TargetType::kHttp) {
    if (!request.method) {
      request.method = "GET";
    }
    if (!request.expected_status_code) {
      request.expected_status_code = 200;
    }
  }

  return request;
}

target_client::UpdateTargetRequest ParseUpdateTargetRequest(
    const userver::formats::json::Value& json) {
  if (!json.IsObject()) {
    throw std::invalid_argument("request body must be a JSON object");
  }

  std::optional<target_client::TargetType> type;
  if (const auto type_value = ReadPatchOptional<std::string>(json, "type")) {
    type = target_client::TargetTypeFromString(*type_value);
  }

  auto expected_status_code =
      ReadPatchOptional<int>(json, "expected_status_code");
  if (!expected_status_code) {
    expected_status_code = ReadPatchOptional<int>(json, "expected_status");
  }

  return target_client::UpdateTargetRequest{
      .name = ReadPatchOptional<std::string>(json, "name"),
      .type = type,
      .url = ReadPatchOptional<std::string>(json, "url"),
      .method = ReadPatchOptional<std::string>(json, "method"),
      .expected_status_code = expected_status_code,
      .host = ReadPatchOptional<std::string>(json, "host"),
      .port = ReadPatchOptional<int>(json, "port"),
      .interval_seconds = ReadPatchOptional<int>(json, "interval_seconds"),
      .timeout_ms = ReadPatchOptional<int>(json, "timeout_ms"),
  };
}

userver::formats::json::Value SerializeTarget(
    const target_client::Target& target) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = target.id;
  builder["name"] = target.name;
  builder["type"] = target_client::TargetTypeToString(target.type);
  common::SetOptionalField(builder, "url", target.url);
  common::SetOptionalField(builder, "method", target.method);
  common::SetOptionalField(builder, "expected_status_code",
                           target.expected_status_code);
  common::SetOptionalField(builder, "host", target.host);
  common::SetOptionalField(builder, "port", target.port);
  builder["interval_seconds"] = target.interval_seconds;
  builder["timeout_ms"] = target.timeout_ms;
  builder["is_active"] = target.is_active;
  return builder.ExtractValue();
}

userver::formats::json::Value SerializeTargets(
    const std::vector<target_client::Target>& targets) {
  userver::formats::json::ValueBuilder builder(
      userver::formats::common::Type::kArray);
  for (const auto& target : targets) {
    userver::formats::json::ValueBuilder item(SerializeTarget(target));
    builder.PushBack(std::move(item));
  }
  return builder.ExtractValue();
}

userver::formats::json::Value SerializeError(std::string_view message) {
  userver::formats::json::ValueBuilder builder;
  builder["error"] = std::string{message};
  return builder.ExtractValue();
}
}  // namespace netwatch::api_gateway::targets
