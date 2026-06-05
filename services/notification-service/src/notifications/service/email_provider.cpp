#include <notifications/service/email_provider.hpp>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <userver/clients/http/error.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>

namespace netwatch::notification_service::notifications {
namespace {

constexpr std::string_view kMailpitProvider = "mailpit";
constexpr std::string_view kYandexPostboxProvider = "yandex-postbox";
constexpr std::string_view kMetadataFlavorHeader = "Google";
constexpr auto kDefaultTokenRefreshSkew = std::chrono::seconds{300};

std::string TrimTrailingSlash(std::string url) {
  while (!url.empty() && url.back() == '/') {
    url.pop_back();
  }
  return url;
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

class MailpitEmailProvider final : public EmailProvider {
 public:
  MailpitEmailProvider(userver::clients::http::Client& http_client,
                       std::string provider_url, std::string from_email,
                       std::string from_name,
                       std::chrono::milliseconds request_timeout)
      : http_client_(http_client),
        provider_url_(TrimTrailingSlash(std::move(provider_url))),
        from_email_(std::move(from_email)),
        from_name_(std::move(from_name)),
        request_timeout_(request_timeout) {
    if (provider_url_.empty()) {
      throw std::invalid_argument{
          "email-delivery-sender.provider-url must be set"};
    }
  }

  void Send(const EmailMessage& message) override {
    const auto response =
        http_client_.CreateRequest()
            .post(provider_url_ + "/api/v1/send", BuildPayload(message))
            .headers({{"Content-Type", "application/json"}})
            .timeout(request_timeout_)
            .retry(0, false)
            .perform();

    ThrowOnBadStatus("mailpit", response->status_code(), response->body());
  }

 private:
  std::string BuildPayload(const EmailMessage& message) const {
    userver::formats::json::ValueBuilder builder;
    builder["From"]["Email"] = from_email_;
    builder["From"]["Name"] = from_name_;

    userver::formats::json::ValueBuilder to;
    userver::formats::json::ValueBuilder recipient;
    recipient["Email"] = message.to_email;
    to.PushBack(recipient.ExtractValue());
    builder["To"] = to.ExtractValue();

    builder["Subject"] = message.subject;
    builder["Text"] = message.text;

    return userver::formats::json::ToString(builder.ExtractValue());
  }

  userver::clients::http::Client& http_client_;
  std::string provider_url_;
  std::string from_email_;
  std::string from_name_;
  std::chrono::milliseconds request_timeout_;
};

class YandexPostboxEmailProvider final : public EmailProvider {
 public:
  YandexPostboxEmailProvider(userver::clients::http::Client& http_client,
                             std::string provider_url, std::string from_email,
                             std::string static_iam_token,
                             std::string metadata_token_url,
                             std::chrono::seconds token_refresh_skew,
                             std::chrono::milliseconds request_timeout)
      : http_client_(http_client),
        provider_url_(TrimTrailingSlash(std::move(provider_url))),
        from_email_(std::move(from_email)),
        static_iam_token_(std::move(static_iam_token)),
        metadata_token_url_(std::move(metadata_token_url)),
        token_refresh_skew_(token_refresh_skew),
        request_timeout_(request_timeout) {
    if (provider_url_.empty()) {
      throw std::invalid_argument{
          "email-delivery-sender.provider-url must be set"};
    }
    if (metadata_token_url_.empty() && static_iam_token_.empty()) {
      throw std::invalid_argument{
          "email-delivery-sender.yandex-postbox-iam-token or "
          "email-delivery-sender.yandex-postbox-metadata-token-url must be "
          "set"};
    }
  }

  void Send(const EmailMessage& message) override {
    const auto response =
        http_client_.CreateRequest()
            .post(provider_url_ + "/v2/email/outbound-emails",
                  BuildPayload(message))
            .headers({{"Content-Type", "application/json"},
                      {"X-YaCloud-SubjectToken", GetIamToken()}})
            .timeout(request_timeout_)
            .retry(0, false)
            .perform();

    ThrowOnBadStatus("yandex-postbox", response->status_code(),
                     response->body());
  }

 private:
  std::string BuildPayload(const EmailMessage& message) const {
    userver::formats::json::ValueBuilder builder;
    builder["FromEmailAddress"] = from_email_;

    userver::formats::json::ValueBuilder to_addresses;
    to_addresses.PushBack(message.to_email);
    builder["Destination"]["ToAddresses"] = to_addresses.ExtractValue();

    builder["Content"]["Simple"]["Subject"]["Data"] = message.subject;
    builder["Content"]["Simple"]["Subject"]["Charset"] = "UTF-8";
    builder["Content"]["Simple"]["Body"]["Text"]["Data"] = message.text;
    builder["Content"]["Simple"]["Body"]["Text"]["Charset"] = "UTF-8";

    return userver::formats::json::ToString(builder.ExtractValue());
  }

  std::string GetIamToken() {
    if (!static_iam_token_.empty()) {
      return static_iam_token_;
    }

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

  userver::clients::http::Client& http_client_;
  std::string provider_url_;
  std::string from_email_;
  std::string static_iam_token_;
  std::string metadata_token_url_;
  std::chrono::seconds token_refresh_skew_;
  std::chrono::milliseconds request_timeout_;
  std::string cached_iam_token_;
  std::chrono::steady_clock::time_point cached_iam_token_expires_at_{};
};

}  // namespace

std::unique_ptr<EmailProvider> MakeEmailProvider(
    const userver::components::ComponentConfig& config,
    userver::clients::http::Client& http_client,
    std::chrono::milliseconds request_timeout) {
  const auto provider = config["provider"].As<std::string>(kMailpitProvider);
  const auto provider_url = config["provider-url"].As<std::string>();
  const auto from_email = config["from-email"].As<std::string>();
  if (from_email.empty()) {
    throw std::invalid_argument{"email-delivery-sender.from-email must be set"};
  }
  const auto from_name = config["from-name"].As<std::string>("NetWatch");

  if (provider == kMailpitProvider) {
    return std::make_unique<MailpitEmailProvider>(
        http_client, provider_url, from_email, from_name, request_timeout);
  }

  if (provider == kYandexPostboxProvider) {
    const auto token_refresh_skew = std::chrono::seconds{
        config["yandex-postbox-token-refresh-skew-sec"].As<int>(
            static_cast<int>(kDefaultTokenRefreshSkew.count()))};
    if (token_refresh_skew.count() < 0) {
      throw std::invalid_argument{
          "email-delivery-sender.yandex-postbox-token-refresh-skew-sec must "
          "be non-negative"};
    }

    return std::make_unique<YandexPostboxEmailProvider>(
        http_client, provider_url, from_email,
        config["yandex-postbox-iam-token"].As<std::string>(""),
        config["yandex-postbox-metadata-token-url"].As<std::string>(""),
        token_refresh_skew, request_timeout);
  }

  throw std::invalid_argument{"unsupported email provider: " + provider};
}

}  // namespace netwatch::notification_service::notifications
