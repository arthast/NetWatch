#pragma once

#include <string_view>
#include <userver/components/component_base.hpp>
#include <userver/kafka/consumer_scope.hpp>

#include <notifications/repository/notification_repository.hpp>

namespace netwatch::notification_service::notifications {

class NotificationConsumer final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "notification-consumer";

  NotificationConsumer(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  void OnAllComponentsAreStopping() override;

  static userver::yaml_config::Schema GetStaticConfigSchema();

 private:
  void ProcessBatch(userver::kafka::MessageBatchView messages) const;

  NotificationRepository notification_repository_;
  bool enabled_{false};

  // Subscription must be the last field because callbacks capture this.
  userver::kafka::ConsumerScope consumer_scope_;
};

}  // namespace netwatch::notification_service::notifications
