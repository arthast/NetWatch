#include <checks/client/check_client.hpp>

#include <chrono>
#include <optional>
#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/ugrpc/client/call_options.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/simple_client_component.hpp>
#include <vector>

namespace monitor_service::checks {
namespace {

namespace proto = netwatch::monitor::v1;
namespace target_proto = netwatch::target::v1;

userver::ugrpc::client::CallOptions MakeCallOptions() {
  userver::ugrpc::client::CallOptions options;
  options.SetAttempts(1);
  options.SetTimeout(std::chrono::milliseconds{1000});
  return options;
}

proto::TargetIdRequest MakeTargetIdRequest(std::int64_t target_id) {
  proto::TargetIdRequest request;
  request.set_target_id(target_id);
  return request;
}

CheckStatus ToDomainCheckStatus(proto::CheckStatus status) {
  switch (status) {
    case proto::CHECK_STATUS_UP:
      return CheckStatus::kUp;
    case proto::CHECK_STATUS_DOWN:
      return CheckStatus::kDown;
    case proto::CHECK_STATUS_UNSPECIFIED:
    case proto::CheckStatus_INT_MIN_SENTINEL_DO_NOT_USE_:
    case proto::CheckStatus_INT_MAX_SENTINEL_DO_NOT_USE_:
      break;
  }

  throw std::invalid_argument("check status must be up or down");
}

target::TargetType ToDomainTargetType(target_proto::TargetType type) {
  switch (type) {
    case target_proto::TARGET_TYPE_HTTP:
      return target::TargetType::kHttp;
    case target_proto::TARGET_TYPE_TCP:
      return target::TargetType::kTcp;
    case target_proto::TARGET_TYPE_UNSPECIFIED:
    case target_proto::TargetType_INT_MIN_SENTINEL_DO_NOT_USE_:
    case target_proto::TargetType_INT_MAX_SENTINEL_DO_NOT_USE_:
      break;
  }

  throw std::invalid_argument("target type must be http or tcp");
}

CheckResult ToDomainCheck(const proto::CheckResult& check) {
  return CheckResult{
      .id = check.id(),
      .target_id = check.target_id(),
      .status = ToDomainCheckStatus(check.status()),
      .protocol = ToDomainTargetType(check.protocol()),
      .http_status = check.has_http_status()
                         ? std::make_optional(check.http_status())
                         : std::nullopt,
      .latency_ms = check.has_latency_ms()
                        ? std::make_optional(check.latency_ms())
                        : std::nullopt,
      .error_message = check.has_error_message()
                           ? std::make_optional(check.error_message())
                           : std::nullopt,
      .checked_at = check.checked_at(),
  };
}

std::vector<CheckResult> ToDomainChecks(
    const proto::ListChecksResponse& response) {
  std::vector<CheckResult> checks;
  checks.reserve(response.checks_size());
  for (const auto& check : response.checks()) {
    checks.push_back(ToDomainCheck(check));
  }
  return checks;
}

}  // namespace

CheckClient::CheckClient(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      client_(context
                  .FindComponent<userver::ugrpc::client::SimpleClientComponent<
                      proto::CheckServiceClient>>("check-service-client")
                  .GetClient()) {}

std::optional<CheckResult> CheckClient::RunCheck(std::int64_t target_id) const {
  try {
    return ToDomainCheck(
        client_.RunCheck(MakeTargetIdRequest(target_id), MakeCallOptions())
            .check());
  } catch (const userver::ugrpc::client::NotFoundError&) {
    return std::nullopt;
  }
}

std::optional<std::vector<CheckResult>> CheckClient::ListTargetChecks(
    std::int64_t target_id) const {
  try {
    return ToDomainChecks(
        client_.ListChecks(MakeTargetIdRequest(target_id), MakeCallOptions()));
  } catch (const userver::ugrpc::client::NotFoundError&) {
    return std::nullopt;
  }
}

std::optional<CheckResult> CheckClient::GetTargetStatus(
    std::int64_t target_id) const {
  try {
    return ToDomainCheck(
        client_
            .GetTargetStatus(MakeTargetIdRequest(target_id), MakeCallOptions())
            .check());
  } catch (const userver::ugrpc::client::NotFoundError&) {
    return std::nullopt;
  }
}

}  // namespace monitor_service::checks
