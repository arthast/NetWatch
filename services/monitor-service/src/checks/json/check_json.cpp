#include <checks/json/check_json.hpp>

#include <optional>
#include <string>
#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace monitor_service::checks {
namespace {
    template<typename T>
    void SetOptional(userver::formats::json::ValueBuilder &builder,
                     std::string_view field, const std::optional<T> &value) {
        if (value) {
            builder[std::string{field}] = *value;
        }
    }
} // namespace

userver::formats::json::Value SerializeCheckResult(const CheckResult &check) {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = check.id;
    builder["target_id"] = check.target_id;
    builder["status"] = CheckStatusToString(check.status);
    builder["protocol"] = target::TargetTypeToString(check.protocol);
    SetOptional(builder, "http_status", check.http_status);
    SetOptional(builder, "latency_ms", check.latency_ms);
    SetOptional(builder, "error_message", check.error_message);
    builder["checked_at"] = check.checked_at;
    return builder.ExtractValue();
}

userver::formats::json::Value SerializeCheckResults(
    const std::vector<CheckResult> &checks) {
    userver::formats::json::ValueBuilder builder(userver::formats::common::Type::kArray);
    for (const auto &check: checks) {
        userver::formats::json::ValueBuilder item(SerializeCheckResult(check));
        builder.PushBack(std::move(item));
    }
    return builder.ExtractValue();
}

} // namespace monitor_service::checks
