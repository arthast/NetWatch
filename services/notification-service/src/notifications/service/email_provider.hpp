#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <string_view>

#include <userver/clients/http/client.hpp>
#include <userver/components/component_config.hpp>

namespace netwatch::notification_service::notifications {

struct EmailMessage final {
  std::string to_email;
  std::string subject;
  std::string text;
};

class EmailProvider {
 public:
  virtual ~EmailProvider() = default;

  virtual void Send(const EmailMessage& message) = 0;
};

std::unique_ptr<EmailProvider> MakeEmailProvider(
    const userver::components::ComponentConfig& config,
    userver::clients::http::Client& http_client,
    std::chrono::milliseconds request_timeout);

}  // namespace netwatch::notification_service::notifications
