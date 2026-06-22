#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <string_view>

#include <userver/clients/http/client.hpp>
#include <userver/concurrent/background_task_storage.hpp>
#include <userver/components/component_base.hpp>
#include <userver/yaml_config/schema.hpp>

namespace netwatch::auth_service::auth {

class EmailVerificationSender final
    : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "email-verification-sender";

  EmailVerificationSender(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  void SendVerificationEmail(std::string_view email,
                             std::string_view token) const;

  static userver::yaml_config::Schema GetStaticConfigSchema();

 private:
  void SendVerificationEmailNow(std::string_view email,
                                std::string_view token) const;
  std::string BuildVerificationUrl(std::string_view token) const;
  std::string BuildPayload(std::string_view email, std::string_view subject,
                           std::string_view text) const;
  std::string GetIamToken() const;

  userver::clients::http::Client& http_client_;
  bool enabled_{false};
  std::string provider_;
  std::string provider_url_;
  std::string from_email_;
  std::string from_name_;
  std::string frontend_base_url_;
  std::string static_iam_token_;
  std::string metadata_token_url_;
  std::chrono::seconds token_refresh_skew_{300};
  std::chrono::milliseconds request_timeout_{3000};
  mutable std::mutex cached_iam_token_mutex_;
  mutable std::string cached_iam_token_;
  mutable std::chrono::steady_clock::time_point cached_iam_token_expires_at_{};
  mutable userver::concurrent::BackgroundTaskStorage background_tasks_;
};

}  // namespace netwatch::auth_service::auth
