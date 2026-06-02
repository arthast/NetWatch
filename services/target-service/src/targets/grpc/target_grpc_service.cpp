#include <targets/grpc/target_grpc_service.hpp>

#include <grpcpp/support/status.h>
#include <optional>
#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/component.hpp>

#include <targets/model/target.hpp>

namespace netwatch::target_service {
namespace {

namespace proto = netwatch::target::v1;
namespace domain = netwatch::target_service;

grpc::Status InvalidArgument(std::string message) {
  return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, std::move(message)};
}

grpc::Status NotFound(std::string message) {
  return grpc::Status{grpc::StatusCode::NOT_FOUND, std::move(message)};
}

proto::TargetType ToProtoTargetType(domain::TargetType type) {
  switch (type) {
    case domain::TargetType::kHttp:
      return proto::TARGET_TYPE_HTTP;
    case domain::TargetType::kTcp:
      return proto::TARGET_TYPE_TCP;
  }
  return proto::TARGET_TYPE_UNSPECIFIED;
}

domain::TargetType ToDomainTargetType(proto::TargetType type) {
  switch (type) {
    case proto::TARGET_TYPE_HTTP:
      return domain::TargetType::kHttp;
    case proto::TARGET_TYPE_TCP:
      return domain::TargetType::kTcp;
    case proto::TARGET_TYPE_UNSPECIFIED:
    case proto::TargetType_INT_MIN_SENTINEL_DO_NOT_USE_:
    case proto::TargetType_INT_MAX_SENTINEL_DO_NOT_USE_:
      break;
  }

  throw std::invalid_argument("target type must be http or tcp");
}

void FillProtoTarget(const domain::Target& source, proto::Target& target) {
  target.set_id(source.id);
  target.set_name(source.name);
  target.set_type(ToProtoTargetType(source.type));

  if (source.url) {
    target.set_url(*source.url);
  }
  if (source.method) {
    target.set_method(*source.method);
  }
  if (source.expected_status_code) {
    target.set_expected_status_code(*source.expected_status_code);
  }
  if (source.host) {
    target.set_host(*source.host);
  }
  if (source.port) {
    target.set_port(*source.port);
  }

  target.set_interval_seconds(source.interval_seconds);
  target.set_timeout_ms(source.timeout_ms);
  target.set_is_active(source.is_active);
}

proto::TargetResponse MakeTargetResponse(const domain::Target& target) {
  proto::TargetResponse response;
  FillProtoTarget(target, *response.mutable_target());
  return response;
}

proto::ListTargetsResponse MakeListTargetsResponse(
    const std::vector<domain::Target>& targets) {
  proto::ListTargetsResponse response;
  for (const auto& target : targets) {
    FillProtoTarget(target, *response.add_targets());
  }
  return response;
}

domain::CreateTargetRequest ToDomainCreateRequest(
    const proto::CreateTargetRequest& request) {
  return domain::CreateTargetRequest{
      .name = request.name(),
      .type = ToDomainTargetType(request.type()),
      .url =
          request.has_url() ? std::make_optional(request.url()) : std::nullopt,
      .method = request.has_method() ? std::make_optional(request.method())
                                     : std::nullopt,
      .expected_status_code =
          request.has_expected_status_code()
              ? std::make_optional(request.expected_status_code())
              : std::nullopt,
      .host = request.has_host() ? std::make_optional(request.host())
                                 : std::nullopt,
      .port = request.has_port() ? std::make_optional(request.port())
                                 : std::nullopt,
      .interval_seconds = request.interval_seconds(),
      .timeout_ms = request.timeout_ms(),
  };
}

domain::UpdateTargetRequest ToDomainUpdateRequest(
    const proto::UpdateTargetRequest& request) {
  return domain::UpdateTargetRequest{
      .name = request.has_name() ? std::make_optional(request.name())
                                 : std::nullopt,
      .type = request.has_type()
                  ? std::make_optional(ToDomainTargetType(request.type()))
                  : std::nullopt,
      .url =
          request.has_url() ? std::make_optional(request.url()) : std::nullopt,
      .method = request.has_method() ? std::make_optional(request.method())
                                     : std::nullopt,
      .expected_status_code =
          request.has_expected_status_code()
              ? std::make_optional(request.expected_status_code())
              : std::nullopt,
      .host = request.has_host() ? std::make_optional(request.host())
                                 : std::nullopt,
      .port = request.has_port() ? std::make_optional(request.port())
                                 : std::nullopt,
      .interval_seconds = request.has_interval_seconds()
                              ? std::make_optional(request.interval_seconds())
                              : std::nullopt,
      .timeout_ms = request.has_timeout_ms()
                        ? std::make_optional(request.timeout_ms())
                        : std::nullopt,
  };
}

}  // namespace

TargetGrpcService::TargetGrpcService(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : proto::TargetServiceBase::Component(config, context),
      target_service_(TargetRepository{
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()}) {}

TargetGrpcService::CreateTargetResult TargetGrpcService::CreateTarget(
    CallContext&, proto::CreateTargetRequest&& request) {
  try {
    return MakeTargetResponse(
        target_service_.CreateTarget(ToDomainCreateRequest(request)));
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

TargetGrpcService::UpdateTargetResult TargetGrpcService::UpdateTarget(
    CallContext&, proto::UpdateTargetRequest&& request) {
  try {
    return MakeTargetResponse(target_service_.UpdateTarget(
        request.id(), ToDomainUpdateRequest(request)));
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  } catch (const TargetNotFound& ex) {
    return NotFound(ex.what());
  }
}

TargetGrpcService::DeleteTargetResult TargetGrpcService::DeleteTarget(
    CallContext&, proto::TargetIdRequest&& request) {
  try {
    target_service_.DeleteTarget(request.id());
    return proto::DeleteTargetResponse{};
  } catch (const TargetNotFound& ex) {
    return NotFound(ex.what());
  }
}

TargetGrpcService::GetTargetResult TargetGrpcService::GetTarget(
    CallContext&, proto::TargetIdRequest&& request) {
  try {
    return MakeTargetResponse(target_service_.GetTarget(request.id()));
  } catch (const TargetNotFound& ex) {
    return NotFound(ex.what());
  }
}

TargetGrpcService::ListTargetsResult TargetGrpcService::ListTargets(
    CallContext&, proto::ListTargetsRequest&&) {
  return MakeListTargetsResponse(target_service_.ListTargets());
}

TargetGrpcService::ListActiveTargetsResult TargetGrpcService::ListActiveTargets(
    CallContext&, proto::ListTargetsRequest&&) {
  return MakeListTargetsResponse(target_service_.ListActiveTargets());
}

}  // namespace netwatch::target_service
