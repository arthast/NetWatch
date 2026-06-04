#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <userver/clients/http/client.hpp>
#include <userver/components/component_base.hpp>
#include <userver/utils/periodic_task.hpp>

#include <notifications/repository/notification_repository.hpp>
#include <notifications/service/email_provider.hpp>

namespace netwatch::notification_service::notifications {

class EmailDeliverySender final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "email-delivery-sender";

  EmailDeliverySender(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  void OnAllComponentsAreStopping() override;

  static userver::yaml_config::Schema GetStaticConfigSchema();

 private:
  void Tick();
  void SendDelivery(const PendingNotificationDelivery& delivery);

  NotificationRepository notification_repository_;
  userver::clients::http::Client& http_client_;
  std::unique_ptr<EmailProvider> email_provider_;
  int batch_size_{0};
  std::chrono::milliseconds request_timeout_{0};
  userver::utils::PeriodicTask periodic_task_;
};

}  // namespace netwatch::notification_service::notifications
