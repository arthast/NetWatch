#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace netwatch::monitor_service::checks {

enum class CheckStatus { kUp, kDown };

enum class CheckProtocol { kHttp, kTcp };

struct CheckResult {
  std::int64_t id{0};
  std::int64_t target_id{0};
  CheckStatus status{CheckStatus::kDown};
  CheckProtocol protocol{CheckProtocol::kHttp};

  std::optional<int> http_status;
  std::optional<int> latency_ms;
  std::optional<std::string> error_message;
  std::string checked_at;
};

std::string CheckStatusToString(CheckStatus status);

CheckStatus CheckStatusFromString(const std::string& value);

std::string CheckProtocolToString(CheckProtocol protocol);

CheckProtocol CheckProtocolFromString(const std::string& value);

}  // namespace netwatch::monitor_service::checks
