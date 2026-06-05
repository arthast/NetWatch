#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace netwatch::notification_client {

struct EmailRecipient {
  std::int64_t id{0};
  std::string email;
  bool is_enabled{false};
  std::string created_at;
  std::string updated_at;
};

struct CreateEmailRecipientRequest {
  std::string email;
};

struct UpdateEmailRecipientRequest {
  std::optional<std::string> email;
  std::optional<bool> is_enabled;
};

}  // namespace netwatch::notification_client
