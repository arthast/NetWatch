#include <checks/json/check_json.hpp>

#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <common/json.hpp>

namespace netwatch::api_gateway::checks {
namespace monitor_client = netwatch::monitor_client;

userver::formats::json::Value SerializeCheckResult(
    const monitor_client::CheckResult& check) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = check.id;
  builder["target_id"] = check.target_id;
  builder["status"] = monitor_client::CheckStatusToString(check.status);
  builder["protocol"] = monitor_client::CheckProtocolToString(check.protocol);
  common::SetOptionalField(builder, "http_status", check.http_status);
  common::SetOptionalField(builder, "latency_ms", check.latency_ms);
  common::SetOptionalField(builder, "error_message", check.error_message);
  builder["checked_at"] = check.checked_at;
  return builder.ExtractValue();
}

userver::formats::json::Value SerializeCheckResults(
    const std::vector<monitor_client::CheckResult>& checks) {
  userver::formats::json::ValueBuilder builder(
      userver::formats::common::Type::kArray);
  for (const auto& check : checks) {
    userver::formats::json::ValueBuilder item(SerializeCheckResult(check));
    builder.PushBack(std::move(item));
  }
  return builder.ExtractValue();
}

}  // namespace netwatch::api_gateway::checks
