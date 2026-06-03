#include <alerts/events/alert_outbox_publisher.hpp>

#include <chrono>
#include <stdexcept>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/kafka/exceptions.hpp>
#include <userver/kafka/producer_component.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

namespace netwatch::alert_service::alerts {
namespace {

constexpr auto kDefaultPublishPeriod = std::chrono::milliseconds{1000};
constexpr auto kDefaultRetryDelay = std::chrono::milliseconds{5000};
constexpr int kDefaultBatchSize = 50;

}  // namespace

AlertOutboxPublisher::AlertOutboxPublisher(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : ComponentBase(config, component_context),
      outbox_repository_(
          component_context
              .FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {
  const auto enabled = config["enabled"].As<bool>(false);
  if (!enabled) {
    LOG_INFO() << "Alert outbox publisher is disabled";
    return;
  }

  topic_ = config["topic"].As<std::string>();
  if (topic_.empty()) {
    throw std::invalid_argument{"alert-outbox-publisher.topic must be set"};
  }

  batch_size_ = config["batch-size"].As<int>(kDefaultBatchSize);
  if (batch_size_ <= 0) {
    throw std::invalid_argument{
        "alert-outbox-publisher.batch-size must be positive"};
  }

  retry_delay_ = std::chrono::milliseconds{config["retry-delay-ms"].As<int>(
      static_cast<int>(kDefaultRetryDelay.count()))};
  if (retry_delay_.count() <= 0) {
    throw std::invalid_argument{
        "alert-outbox-publisher.retry-delay-ms must be positive"};
  }

  const auto publish_period =
      std::chrono::milliseconds{config["publish-period-ms"].As<int>(
          static_cast<int>(kDefaultPublishPeriod.count()))};
  if (publish_period.count() <= 0) {
    throw std::invalid_argument{
        "alert-outbox-publisher.publish-period-ms must be positive"};
  }

  producer_ = &component_context
                   .FindComponent<userver::kafka::ProducerComponent>(
                       "kafka-producer-alert-events")
                   .GetProducer();

  auto& testsuite_tasks =
      component_context.FindComponent<userver::components::TestsuiteSupport>()
          .GetTestsuiteTasks();
  userver::utils::StartPeriodicTask(
      periodic_task_, std::string{kName},
      userver::utils::PeriodicTask::Settings{
          publish_period, {userver::utils::PeriodicTask::Flags::kStrong}},
      [this] { Tick(); }, testsuite_tasks);
}

void AlertOutboxPublisher::OnAllComponentsAreStopping() {
  periodic_task_.Stop();
}

userver::yaml_config::Schema AlertOutboxPublisher::GetStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(
      R"(
type: object
description: publishes alert outbox events to Kafka
additionalProperties: false
properties:
    enabled:
        type: boolean
        description: enables publishing pending alert outbox events
        defaultDescription: false
    topic:
        type: string
        description: Kafka topic for alert events
    batch-size:
        type: integer
        description: maximum number of outbox events to acquire per tick
        defaultDescription: 50
    publish-period-ms:
        type: integer
        description: outbox polling period in milliseconds
        defaultDescription: 1000
    retry-delay-ms:
        type: integer
        description: delay before retrying failed publish attempts
        defaultDescription: 5000
)");
}

void AlertOutboxPublisher::Tick() {
  const auto events = outbox_repository_.AcquirePendingEvents(batch_size_);
  for (const auto& event : events) {
    try {
      producer_->Send(topic_, event.partition_key, event.payload);
      outbox_repository_.MarkPublished(event.event_id);
      LOG_INFO() << "Published alert outbox event, event_id=" << event.event_id
                 << ", event_type=" << AlertEventTypeToString(event.event_type)
                 << ", topic=" << topic_;
    } catch (const userver::kafka::SendException& ex) {
      outbox_repository_.MarkFailed(event.event_id, ex.what(), retry_delay_);
      LOG_WARNING() << "Failed to publish alert outbox event, "
                    << "event_id=" << event.event_id
                    << ", retryable=" << ex.IsRetryable()
                    << ", error=" << ex.what();
    } catch (const std::exception& ex) {
      outbox_repository_.MarkFailed(event.event_id, ex.what(), retry_delay_);
      LOG_WARNING() << "Failed to publish alert outbox event, "
                    << "event_id=" << event.event_id << ", error=" << ex.what();
    }
  }
}

}  // namespace netwatch::alert_service::alerts
