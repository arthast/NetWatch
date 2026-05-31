#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace netwatch::alert_client {

enum class AlertType { kTargetDown, kTargetRecovered, kHighLatency };

enum class AlertSeverity { kWarning, kCritical };

struct Alert {
  std::int64_t id{0};
  std::int64_t target_id{0};
  AlertType type{AlertType::kTargetDown};
  AlertSeverity severity{AlertSeverity::kCritical};
  std::string message;
  std::string created_at;
  std::optional<std::string> resolved_at;
};

std::string AlertTypeToString(AlertType type);

AlertType AlertTypeFromString(const std::string& value);

std::string AlertSeverityToString(AlertSeverity severity);

AlertSeverity AlertSeverityFromString(const std::string& value);

}  // namespace netwatch::alert_client
