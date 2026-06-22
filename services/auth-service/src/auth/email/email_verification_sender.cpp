#include <auth/email/email_verification_sender.hpp>

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>

#include <userver/clients/http/component.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

namespace netwatch::auth_service::auth {
namespace {

constexpr std::string_view kMailpitProvider = "mailpit";
constexpr std::string_view kYandexPostboxProvider = "yandex-postbox";
constexpr std::string_view kMetadataFlavorHeader = "Google";

std::string TrimTrailingSlash(std::string value) {
  while (!value.empty() && value.back() == '/') {
    value.pop_back();
  }
  return value;
}

void ThrowOnBadStatus(std::string_view provider_name,
                      userver::clients::http::Status status,
                      std::string_view body) {
  const auto status_code = static_cast<int>(status);
  if (status_code >= 200 && status_code < 300) {
    return;
  }

  throw std::runtime_error{std::string{provider_name} + " returned HTTP " +
                           std::to_string(status_code) + ": " +
                           std::string{body}};
}

}  // namespace

EmailVerificationSender::EmailVerificationSender(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : ComponentBase(config, component_context),
      http_client_(
          component_context.FindComponent<userver::components::HttpClient>()
              .GetHttpClient()) {
  enabled_ = config["enabled"].As<bool>(false);
  provider_ = config["provider"].As<std::string>(std::string{kMailpitProvider});
  provider_url_ = TrimTrailingSlash(config["provider-url"].As<std::string>(""));
  from_email_ = config["from-email"].As<std::string>("");
  from_name_ = config["from-name"].As<std::string>("NetWatch");
  frontend_base_url_ =
      TrimTrailingSlash(config["frontend-base-url"].As<std::string>(""));
  static_iam_token_ = config["yandex-postbox-iam-token"].As<std::string>("");
  metadata_token_url_ =
      config["yandex-postbox-metadata-token-url"].As<std::string>("");
  token_refresh_skew_ = std::chrono::seconds{
      config["yandex-postbox-token-refresh-skew-sec"].As<int>(300)};
  request_timeout_ = std::chrono::milliseconds{
      config["request-timeout-ms"].As<int>(3000)};

  if (!enabled_) {
    LOG_INFO() << "Email verification sender is disabled";
    return;
  }

  if (provider_url_.empty()) {
    throw std::invalid_argument{
        "email-verification-sender.provider-url must be set"};
  }
  if (from_email_.empty()) {
    throw std::invalid_argument{
        "email-verification-sender.from-email must be set"};
  }
  if (frontend_base_url_.empty()) {
    throw std::invalid_argument{
        "email-verification-sender.frontend-base-url must be set"};
  }
  if (request_timeout_.count() <= 0) {
    throw std::invalid_argument{
        "email-verification-sender.request-timeout-ms must be positive"};
  }
  if (provider_ == kYandexPostboxProvider && static_iam_token_.empty() &&
      metadata_token_url_.empty()) {
    throw std::invalid_argument{
        "email-verification-sender.yandex-postbox-iam-token or "
        "email-verification-sender.yandex-postbox-metadata-token-url must be "
        "set"};
  }
}

void EmailVerificationSender::SendVerificationEmail(
    std::string_view email, std::string_view token) const {
  if (!enabled_) {
    LOG_INFO() << "Skipping email verification message because sender is "
                  "disabled, email="
               << email;
    return;
  }

  background_tasks_.AsyncDetach(
      "auth-email-verification-send",
      [this, email = std::string{email}, token = std::string{token}] {
        try {
          SendVerificationEmailNow(email, token);
        } catch (const std::exception& ex) {
          LOG_WARNING() << "Failed to send email verification message, email="
                        << email << ", error=" << ex.what();
        }
      });
}

void EmailVerificationSender::SendVerificationEmailNow(
    std::string_view email, std::string_view token) const {
  const auto url = BuildVerificationUrl(token);
  const std::string subject = "[NetWatch] Verify your email";
  std::string text;
  text += "NetWatch email verification\n\n";
  text += "Use this link to verify your email address:\n";
  text += url;
  text += "\n\nThis link expires in 24 hours.\n";

  const auto payload = BuildPayload(email, subject, text);
  if (provider_ == kMailpitProvider) {
    const auto response =
        http_client_.CreateRequest()
            .post(provider_url_ + "/api/v1/send", payload)
            .headers({{"Content-Type", "application/json"}})
            .timeout(request_timeout_)
            .retry(0, false)
            .perform();
    ThrowOnBadStatus("mailpit", response->status_code(), response->body());
    return;
  }

  if (provider_ == kYandexPostboxProvider) {
    const auto response =
        http_client_.CreateRequest()
            .post(provider_url_ + "/v2/email/outbound-emails", payload)
            .headers({{"Content-Type", "application/json"},
                      {"X-YaCloud-SubjectToken", GetIamToken()}})
            .timeout(request_timeout_)
            .retry(0, false)
            .perform();
    ThrowOnBadStatus("yandex-postbox", response->status_code(),
                     response->body());
    return;
  }

  throw std::invalid_argument{"unsupported email provider: " + provider_};
}

std::string EmailVerificationSender::BuildVerificationUrl(
    std::string_view token) const {
  return frontend_base_url_ + "/verify-email?token=" + std::string{token};
}

std::string EmailVerificationSender::BuildPayload(
    std::string_view email, std::string_view subject,
    std::string_view text) const {
  if (provider_ == kMailpitProvider) {
    userver::formats::json::ValueBuilder builder;
    builder["From"]["Email"] = from_email_;
    builder["From"]["Name"] = from_name_;

    userver::formats::json::ValueBuilder to;
    userver::formats::json::ValueBuilder recipient;
    recipient["Email"] = std::string{email};
    to.PushBack(recipient.ExtractValue());
    builder["To"] = to.ExtractValue();
    builder["Subject"] = std::string{subject};
    builder["Text"] = std::string{text};
    return userver::formats::json::ToString(builder.ExtractValue());
  }

  userver::formats::json::ValueBuilder builder;
  builder["FromEmailAddress"] = from_email_;

  userver::formats::json::ValueBuilder to_addresses;
  to_addresses.PushBack(std::string{email});
  builder["Destination"]["ToAddresses"] = to_addresses.ExtractValue();
  builder["Content"]["Simple"]["Subject"]["Data"] = std::string{subject};
  builder["Content"]["Simple"]["Subject"]["Charset"] = "UTF-8";
  builder["Content"]["Simple"]["Body"]["Text"]["Data"] = std::string{text};
  builder["Content"]["Simple"]["Body"]["Text"]["Charset"] = "UTF-8";
  return userver::formats::json::ToString(builder.ExtractValue());
}

std::string EmailVerificationSender::GetIamToken() const {
  if (!static_iam_token_.empty()) {
    return static_iam_token_;
  }

  const std::lock_guard lock{cached_iam_token_mutex_};
  const auto now = std::chrono::steady_clock::now();
  if (!cached_iam_token_.empty() && now < cached_iam_token_expires_at_) {
    return cached_iam_token_;
  }

  const auto response =
      http_client_.CreateRequest()
          .get(metadata_token_url_)
          .headers({{"Metadata-Flavor", std::string{kMetadataFlavorHeader}}})
          .timeout(request_timeout_)
          .retry(0, false)
          .perform();

  ThrowOnBadStatus("yandex-metadata", response->status_code(),
                   response->body());

  const auto body = userver::formats::json::FromString(response->body());
  cached_iam_token_ = body["access_token"].As<std::string>();
  const auto expires_in = std::chrono::seconds{
      body["expires_in"].As<int>(static_cast<int>(3600))};
  const auto refresh_in = expires_in > token_refresh_skew_
                              ? expires_in - token_refresh_skew_
                              : std::chrono::seconds{0};
  cached_iam_token_expires_at_ = now + refresh_in;

  if (cached_iam_token_.empty()) {
    throw std::runtime_error{"Yandex metadata returned an empty IAM token"};
  }

  return cached_iam_token_;
}

userver::yaml_config::Schema
EmailVerificationSender::GetStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(
      R"(
type: object
description: sends auth email verification messages
additionalProperties: false
properties:
    enabled:
        type: boolean
        description: enables verification email sending
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
        defaultDescription: empty
    from-email:
        type: string
        description: sender email address
        defaultDescription: empty
    from-name:
        type: string
        description: sender display name
        defaultDescription: NetWatch
    frontend-base-url:
        type: string
        description: public frontend base URL used in verification links
        defaultDescription: empty
    request-timeout-ms:
        type: integer
        description: email provider HTTP request timeout in milliseconds
        defaultDescription: 3000
    yandex-postbox-iam-token:
        type: string
        description: optional IAM token for Yandex Cloud Postbox
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

}  // namespace netwatch::auth_service::auth
