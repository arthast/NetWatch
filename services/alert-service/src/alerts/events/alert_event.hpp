#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace netwatch::alert_service::alerts {

enum class AlertEventType { kAlertOpened, kAlertResolved };

enum class AlertOutboxStatus { kPending, kPublishing, kPublished, kFailed };

struct AlertEventTargetSnapshot {
  std::int64_t id{0};
  std::optional<std::int64_t> user_id;
  std::string name;
  std::string type;
};

struct AlertOutboxEvent {
  std::string event_id;
  AlertEventType event_type{AlertEventType::kAlertOpened};
  std::string aggregate_type;
  std::int64_t aggregate_id{0};
  std::string partition_key;
  std::string payload;
  AlertOutboxStatus status{AlertOutboxStatus::kPending};
  int attempts{0};
  std::string next_retry_at;
  std::optional<std::string> last_error;
  std::string created_at;
  std::optional<std::string> published_at;
};

std::string AlertEventTypeToString(AlertEventType type);

AlertEventType AlertEventTypeFromString(const std::string& value);

std::string AlertOutboxStatusToString(AlertOutboxStatus status);

AlertOutboxStatus AlertOutboxStatusFromString(const std::string& value);

}  // namespace netwatch::alert_service::alerts
