#include <checks/grpc/check_grpc_service.hpp>

#include <grpcpp/support/status.h>
#include <string>
#include <userver/components/component_context.hpp>
#include <utility>
#include <vector>

namespace monitor_service::checks {
namespace {

namespace proto = netwatch::monitor::v1;
namespace target_proto = netwatch::target::v1;

grpc::Status NotFound(std::string message) {
  return grpc::Status{grpc::StatusCode::NOT_FOUND, std::move(message)};
}

proto::CheckStatus ToProtoCheckStatus(CheckStatus status) {
  switch (status) {
    case CheckStatus::kUp:
      return proto::CHECK_STATUS_UP;
    case CheckStatus::kDown:
      return proto::CHECK_STATUS_DOWN;
  }
  return proto::CHECK_STATUS_UNSPECIFIED;
}

target_proto::TargetType ToProtoTargetType(target::TargetType type) {
  switch (type) {
    case target::TargetType::kHttp:
      return target_proto::TARGET_TYPE_HTTP;
    case target::TargetType::kTcp:
      return target_proto::TARGET_TYPE_TCP;
  }
  return target_proto::TARGET_TYPE_UNSPECIFIED;
}

void FillProtoCheck(const CheckResult& source, proto::CheckResult& target) {
  target.set_id(source.id);
  target.set_target_id(source.target_id);
  target.set_status(ToProtoCheckStatus(source.status));
  target.set_protocol(ToProtoTargetType(source.protocol));

  if (source.http_status) {
    target.set_http_status(*source.http_status);
  }
  if (source.latency_ms) {
    target.set_latency_ms(*source.latency_ms);
  }
  if (source.error_message) {
    target.set_error_message(*source.error_message);
  }

  target.set_checked_at(source.checked_at);
}

proto::CheckResultResponse MakeCheckResponse(const CheckResult& check) {
  proto::CheckResultResponse response;
  FillProtoCheck(check, *response.mutable_check());
  return response;
}

proto::ListChecksResponse MakeChecksResponse(
    const std::vector<CheckResult>& checks) {
  proto::ListChecksResponse response;
  for (const auto& check : checks) {
    FillProtoCheck(check, *response.add_checks());
  }
  return response;
}

}  // namespace

CheckGrpcService::CheckGrpcService(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : proto::CheckServiceBase::Component(config, context),
      check_service_(context.FindComponent<CheckServiceComponent>()) {}

CheckGrpcService::RunCheckResult CheckGrpcService::RunCheck(
    CallContext&, proto::TargetIdRequest&& request) {
  const auto check = check_service_.RunCheckForTarget(request.target_id());
  if (!check) {
    return NotFound("target not found");
  }

  return MakeCheckResponse(*check);
}

CheckGrpcService::ListChecksResult CheckGrpcService::ListChecks(
    CallContext&, proto::TargetIdRequest&& request) {
  const auto checks = check_service_.ListTargetChecks(request.target_id());
  if (!checks) {
    return NotFound("target not found");
  }

  return MakeChecksResponse(*checks);
}

CheckGrpcService::GetTargetStatusResult CheckGrpcService::GetTargetStatus(
    CallContext&, proto::TargetIdRequest&& request) {
  const auto status = check_service_.GetTargetStatus(request.target_id());
  if (!status) {
    return NotFound("target has no checks");
  }

  return MakeCheckResponse(*status);
}

}  // namespace monitor_service::checks
