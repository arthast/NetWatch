#include <alerts/events/alert_event.hpp>

#include <stdexcept>

namespace netwatch::alert_service::alerts {

std::string AlertEventTypeToString(AlertEventType type) {
  switch (type) {
    case AlertEventType::kAlertOpened:
      return "alert.opened";
    case AlertEventType::kAlertResolved:
      return "alert.resolved";
  }

  throw std::invalid_argument("unknown alert event type");
}

AlertEventType AlertEventTypeFromString(const std::string& value) {
  if (value == "alert.opened") {
    return AlertEventType::kAlertOpened;
  }
  if (value == "alert.resolved") {
    return AlertEventType::kAlertResolved;
  }

  throw std::invalid_argument("unknown alert event type: " + value);
}

std::string AlertOutboxStatusToString(AlertOutboxStatus status) {
  switch (status) {
    case AlertOutboxStatus::kPending:
      return "pending";
    case AlertOutboxStatus::kPublishing:
      return "publishing";
    case AlertOutboxStatus::kPublished:
      return "published";
    case AlertOutboxStatus::kFailed:
      return "failed";
  }

  throw std::invalid_argument("unknown alert outbox status");
}

AlertOutboxStatus AlertOutboxStatusFromString(const std::string& value) {
  if (value == "pending") {
    return AlertOutboxStatus::kPending;
  }
  if (value == "publishing") {
    return AlertOutboxStatus::kPublishing;
  }
  if (value == "published") {
    return AlertOutboxStatus::kPublished;
  }
  if (value == "failed") {
    return AlertOutboxStatus::kFailed;
  }

  throw std::invalid_argument("unknown alert outbox status: " + value);
}

}  // namespace netwatch::alert_service::alerts
