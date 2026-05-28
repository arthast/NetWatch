#include <checks/json/check_json.hpp>

#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <common/json.hpp>

namespace monitor_service::checks {

userver::formats::json::Value SerializeCheckResult(const CheckResult& check) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = check.id;
  builder["target_id"] = check.target_id;
  builder["status"] = CheckStatusToString(check.status);
  builder["protocol"] = target::TargetTypeToString(check.protocol);
  common::SetOptionalField(builder, "http_status", check.http_status);
  common::SetOptionalField(builder, "latency_ms", check.latency_ms);
  common::SetOptionalField(builder, "error_message", check.error_message);
  builder["checked_at"] = check.checked_at;
  return builder.ExtractValue();
}

userver::formats::json::Value SerializeCheckResults(
    const std::vector<CheckResult>& checks) {
  userver::formats::json::ValueBuilder builder(
      userver::formats::common::Type::kArray);
  for (const auto& check : checks) {
    userver::formats::json::ValueBuilder item(SerializeCheckResult(check));
    builder.PushBack(std::move(item));
  }
  return builder.ExtractValue();
}

}  // namespace monitor_service::checks
