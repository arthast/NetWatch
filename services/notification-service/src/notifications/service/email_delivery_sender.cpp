#include <notifications/service/email_delivery_sender.hpp>

#include <chrono>
#include <stdexcept>
#include <userver/clients/http/component.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

namespace netwatch::notification_service::notifications {
namespace {

constexpr auto kDefaultSendPeriod = std::chrono::milliseconds{1000};
constexpr auto kDefaultRequestTimeout = std::chrono::milliseconds{3000};
constexpr int kDefaultBatchSize = 20;

std::string AlertSubjectPrefix(std::string_view event_type) {
  if (event_type == "alert.opened") {
    return "Target is down";
  }
  if (event_type == "alert.resolved") {
    return "Target recovered";
  }
  return "Alert update";
}

std::string AlertStatusTitle(std::string_view event_type) {
  if (event_type == "alert.opened") {
    return "DOWN";
  }
  if (event_type == "alert.resolved") {
    return "RECOVERED";
  }
  return "UPDATED";
}

std::string ReadStringOr(const userver::formats::json::Value& json,
                         std::string_view field, std::string fallback = {}) {
  const auto value = json[field];
  if (value.IsMissing() || value.IsNull()) {
    return fallback;
  }
  return value.As<std::string>();
}

std::string AlertMessage(const userver::formats::json::Value& alert,
                         std::string_view event_type,
                         std::string_view target_name) {
  if (event_type == "alert.resolved") {
    return "Target " + std::string{target_name} + " recovered";
  }

  const auto message = ReadStringOr(alert, "message");
  if (!message.empty()) {
    return message;
  }

  if (event_type == "alert.opened") {
    return "Target " + std::string{target_name} + " is down";
  }
  return {};
}

std::string BuildSubject(const PendingNotificationDelivery& delivery,
                         const userver::formats::json::Value& payload) {
  const auto target = payload["target"];
  const auto target_name = ReadStringOr(target, "name", "target");
  return "[NetWatch] " + AlertSubjectPrefix(delivery.event_type) + ": " +
         target_name;
}

std::string BuildText(const PendingNotificationDelivery& delivery,
                      const userver::formats::json::Value& payload) {
  const auto alert = payload["alert"];
  const auto target = payload["target"];

  std::string text;
  const auto target_name = ReadStringOr(target, "name", "target");
  text += "NetWatch alert\n\n";
  text += "Target: " + target_name + "\n";
  text += "Status: " + AlertStatusTitle(delivery.event_type) + "\n";
  text += "Severity: " + ReadStringOr(alert, "severity", "unknown") + "\n";
  text += "Type: " + ReadStringOr(alert, "type", "unknown") + "\n";
  if (const auto message = AlertMessage(alert, delivery.event_type, target_name);
      !message.empty()) {
    text += "Message: " + message + "\n";
  }
  if (const auto occurred_at = ReadStringOr(payload, "occurred_at");
      !occurred_at.empty()) {
    text += "Time: " + occurred_at + "\n";
  }
  text += "\nTechnical details\n";
  text += "Event: " + delivery.event_type + "\n";
  text += "Event ID: " + delivery.event_id + "\n";
  return text;
}

EmailMessage BuildEmailMessage(const PendingNotificationDelivery& delivery) {
  const auto payload = userver::formats::json::FromString(delivery.payload);
  return EmailMessage{delivery.recipient_email, BuildSubject(delivery, payload),
                      BuildText(delivery, payload)};
}

}  // namespace

EmailDeliverySender::EmailDeliverySender(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : ComponentBase(config, component_context),
      notification_repository_(
          component_context
              .FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()),
      http_client_(
          component_context.FindComponent<userver::components::HttpClient>()
              .GetHttpClient()) {
  const auto enabled = config["enabled"].As<bool>(false);
  if (!enabled) {
    LOG_INFO() << "Email delivery sender is disabled";
    return;
  }

  batch_size_ = config["batch-size"].As<int>(kDefaultBatchSize);
  if (batch_size_ <= 0) {
    throw std::invalid_argument{
        "email-delivery-sender.batch-size must be positive"};
  }

  request_timeout_ =
      std::chrono::milliseconds{config["request-timeout-ms"].As<int>(
          static_cast<int>(kDefaultRequestTimeout.count()))};
  if (request_timeout_.count() <= 0) {
    throw std::invalid_argument{
        "email-delivery-sender.request-timeout-ms must be positive"};
  }

  email_provider_ = MakeEmailProvider(config, http_client_, request_timeout_);

  const auto send_period =
      std::chrono::milliseconds{config["send-period-ms"].As<int>(
          static_cast<int>(kDefaultSendPeriod.count()))};
  if (send_period.count() <= 0) {
    throw std::invalid_argument{
        "email-delivery-sender.send-period-ms must be positive"};
  }

  if (const auto seed_recipient =
          config["seed-recipient-email"].As<std::string>("");
      !seed_recipient.empty()) {
    notification_repository_.EnsureRecipient(seed_recipient);
  }

  auto& testsuite_tasks =
      component_context.FindComponent<userver::components::TestsuiteSupport>()
          .GetTestsuiteTasks();
  userver::utils::StartPeriodicTask(
      periodic_task_, std::string{kName},
      userver::utils::PeriodicTask::Settings{
          send_period, {userver::utils::PeriodicTask::Flags::kStrong}},
      [this] { Tick(); }, testsuite_tasks);
}

void EmailDeliverySender::OnAllComponentsAreStopping() {
  periodic_task_.Stop();
}

userver::yaml_config::Schema EmailDeliverySender::GetStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(
      R"(
type: object
description: sends pending email notification deliveries through an HTTP email provider
additionalProperties: false
properties:
    enabled:
        type: boolean
        description: enables sending pending email notification deliveries
        defaultDescription: false
    provider:
        type: string
        description: email provider implementation
        enum:
          - mailpit
          - yandex-postbox
        defaultDescription: mailpit
    provider-url:
        type: string
        description: base URL of the email provider HTTP API
    from-email:
        type: string
        description: sender email address
    from-name:
        type: string
        description: sender display name
        defaultDescription: NetWatch
    seed-recipient-email:
        type: string
        description: optional recipient inserted on startup for local/dev environments
        defaultDescription: empty
    batch-size:
        type: integer
        description: maximum pending deliveries to send per tick
        defaultDescription: 20
    send-period-ms:
        type: integer
        description: pending delivery polling period in milliseconds
        defaultDescription: 1000
    request-timeout-ms:
        type: integer
        description: email provider HTTP request timeout in milliseconds
        defaultDescription: 3000
    yandex-postbox-iam-token:
        type: string
        description: optional IAM token for Yandex Cloud Postbox; metadata token URL is used when empty
        defaultDescription: empty
    yandex-postbox-metadata-token-url:
        type: string
        description: metadata URL used to obtain a Yandex Cloud service account IAM token
        defaultDescription: empty
    yandex-postbox-token-refresh-skew-sec:
        type: integer
        description: seconds before token expiration when the cached IAM token is refreshed
        defaultDescription: 300
)");
}

void EmailDeliverySender::Tick() {
  const auto deliveries =
      notification_repository_.AcquirePendingDeliveries(batch_size_);

  for (const auto& delivery : deliveries) {
    try {
      SendDelivery(delivery);
      notification_repository_.MarkDeliverySent(delivery.id);
      LOG_INFO() << "Sent notification email delivery, delivery_id="
                 << delivery.id << ", event_id=" << delivery.event_id
                 << ", recipient=" << delivery.recipient_email;
    } catch (const std::exception& ex) {
      notification_repository_.MarkDeliveryFailed(delivery.id, ex.what());
      LOG_WARNING() << "Failed to send notification email delivery, "
                    << "delivery_id=" << delivery.id
                    << ", event_id=" << delivery.event_id
                    << ", error=" << ex.what();
    }
  }
}

void EmailDeliverySender::SendDelivery(
    const PendingNotificationDelivery& delivery) {
  email_provider_->Send(BuildEmailMessage(delivery));
}

}  // namespace netwatch::notification_service::notifications
