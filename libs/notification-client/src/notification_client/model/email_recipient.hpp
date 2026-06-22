#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace netwatch::notification_client {

struct EmailRecipient {
  std::int64_t id{0};
  std::optional<std::int64_t> user_id;
  std::string email;
  bool is_enabled{false};
  std::string created_at;
  std::string updated_at;
};

struct CreateEmailRecipientRequest {
  std::optional<std::int64_t> user_id;
  std::string email;
};

struct UpdateEmailRecipientRequest {
  std::optional<std::int64_t> user_id;
  std::optional<bool> is_enabled;
};

struct TargetNotificationSettings {
  std::int64_t user_id{0};
  std::int64_t target_id{0};
  bool email_enabled{true};
  std::string created_at;
  std::string updated_at;
};

}  // namespace netwatch::notification_client
