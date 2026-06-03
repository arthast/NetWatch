#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <userver/components/component_base.hpp>
#include <userver/kafka/producer.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/utils/periodic_task.hpp>

#include <alerts/repository/alert_outbox_repository.hpp>

namespace netwatch::alert_service::alerts {

class AlertOutboxPublisher final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "alert-outbox-publisher";

  AlertOutboxPublisher(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context);

  void OnAllComponentsAreStopping() override;

  static userver::yaml_config::Schema GetStaticConfigSchema();

 private:
  void Tick();

  AlertOutboxRepository outbox_repository_;
  const userver::kafka::Producer* producer_{nullptr};
  std::string topic_;
  int batch_size_{0};
  std::chrono::milliseconds retry_delay_{0};
  userver::utils::PeriodicTask periodic_task_;
};

}  // namespace netwatch::alert_service::alerts
