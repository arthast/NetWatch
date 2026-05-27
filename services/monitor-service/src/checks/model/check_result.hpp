#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <targets/model/target.hpp>

namespace monitor_service::checks {

enum class CheckStatus { kUp, kDown };

struct CheckResult {
  std::int64_t id{0};
  std::int64_t target_id{0};
  CheckStatus status{CheckStatus::kDown};
  target::TargetType protocol{target::TargetType::kHttp};

  std::optional<int> http_status;
  std::optional<int> latency_ms;
  std::optional<std::string> error_message;
  std::string checked_at;
};

std::string CheckStatusToString(CheckStatus status);

CheckStatus CheckStatusFromString(const std::string& value);

}  // namespace monitor_service::checks
