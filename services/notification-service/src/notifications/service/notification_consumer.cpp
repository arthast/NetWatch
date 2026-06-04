#include <notifications/service/notification_consumer.hpp>

#include <stdexcept>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/kafka/consumer_component.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

#include <notifications/events/alert_event.hpp>

namespace netwatch::notification_service::notifications {
NotificationConsumer::NotificationConsumer(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &component_context)
    : ComponentBase(config, component_context),
      notification_repository_(
          component_context
          .FindComponent<userver::components::Postgres>("postgres-db-1")
          .GetCluster()),
      enabled_(config["enabled"].As<bool>(false)),
      consumer_scope_(component_context
          .FindComponent<userver::kafka::ConsumerComponent>(
              "kafka-consumer-alert-events")
          .GetConsumer()) {
    if (!enabled_) {
        LOG_INFO() << "Notification Kafka consumer is disabled";
        return;
    }

    consumer_scope_.Start([this](userver::kafka::MessageBatchView messages) {
        ProcessBatch(messages);
        consumer_scope_.AsyncCommit();
    });
}

void NotificationConsumer::OnAllComponentsAreStopping() {
    consumer_scope_.Stop();
}

userver::yaml_config::Schema NotificationConsumer::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(
        R"(
type: object
description: consumes alert lifecycle events and prepares notification delivery
additionalProperties: false
properties:
    enabled:
        type: boolean
        description: enables consuming Kafka alert lifecycle events
        defaultDescription: false
)");
}

void NotificationConsumer::ProcessBatch(
    userver::kafka::MessageBatchView messages) const {
    for (const auto &message: messages) {
        try {
            const auto event = ParseAlertEvent(message.GetPayload());
            const auto result = notification_repository_.ProcessAlertEvent(event);

            if (result.inserted) {
                LOG_INFO() << "Processed alert notification event, "
                        << "event_id=" << event.event_id
                        << ", event_type=" << event.event_type
                        << ", deliveries=" << result.deliveries_count
                        << ", recipients=" << result.recipients_count;
            } else {
                LOG_DEBUG() << "Skipped duplicate alert notification event, "
                        << "event_id=" << event.event_id;
            }
        } catch (const std::exception &ex) {
            LOG_WARNING() << "Failed to process alert notification event, "
                    << "topic=" << message.GetTopic()
                    << ", partition=" << message.GetPartition()
                    << ", offset=" << message.GetOffset()
                    << ", error=" << ex.what();
            throw;
        }
    }
}
} // namespace netwatch::notification_service::notifications
