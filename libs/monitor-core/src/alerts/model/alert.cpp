#include <alerts/model/alert.hpp>

#include <stdexcept>

namespace monitor_service::alerts {

std::string AlertTypeToString(AlertType type) {
  switch (type) {
    case AlertType::kTargetDown:
      return "target_down";
    case AlertType::kTargetRecovered:
      return "target_recovered";
    case AlertType::kHighLatency:
      return "high_latency";
  }

  throw std::invalid_argument("unknown alert type");
}

AlertType AlertTypeFromString(const std::string& value) {
  if (value == "target_down") {
    return AlertType::kTargetDown;
  }
  if (value == "target_recovered") {
    return AlertType::kTargetRecovered;
  }
  if (value == "high_latency") {
    return AlertType::kHighLatency;
  }

  throw std::invalid_argument("unknown alert type: " + value);
}

std::string AlertSeverityToString(AlertSeverity severity) {
  switch (severity) {
    case AlertSeverity::kWarning:
      return "warning";
    case AlertSeverity::kCritical:
      return "critical";
  }

  throw std::invalid_argument("unknown alert severity");
}

AlertSeverity AlertSeverityFromString(const std::string& value) {
  if (value == "warning") {
    return AlertSeverity::kWarning;
  }
  if (value == "critical") {
    return AlertSeverity::kCritical;
  }

  throw std::invalid_argument("unknown alert severity: " + value);
}

}  // namespace monitor_service::alerts
