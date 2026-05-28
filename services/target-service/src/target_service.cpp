#include <target_service.hpp>

#include <grpcpp/support/status.h>
#include <optional>
#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/component.hpp>

#include <targets/model/target.hpp>
#include <targets/validation/target_validator.hpp>

namespace netwatch::target_service {
namespace {

namespace proto = netwatch::target::v1;
namespace domain = monitor_service::target;

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
  auto result = domain::CreateTargetRequest{
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

  if (result.type == domain::TargetType::kHttp) {
    if (!result.method) {
      result.method = "GET";
    }
    if (!result.expected_status_code) {
      result.expected_status_code = 200;
    }
  }

  return result;
}

domain::Target ToDomainTarget(const proto::Target& target) {
  return domain::Target{
      .id = target.id(),
      .name = target.name(),
      .type = ToDomainTargetType(target.type()),
      .url = target.has_url() ? std::make_optional(target.url()) : std::nullopt,
      .method = target.has_method() ? std::make_optional(target.method())
                                    : std::nullopt,
      .expected_status_code =
          target.has_expected_status_code()
              ? std::make_optional(target.expected_status_code())
              : std::nullopt,
      .host =
          target.has_host() ? std::make_optional(target.host()) : std::nullopt,
      .port =
          target.has_port() ? std::make_optional(target.port()) : std::nullopt,
      .interval_seconds = target.interval_seconds(),
      .timeout_ms = target.timeout_ms(),
      .is_active = target.is_active(),
  };
}

}  // namespace

TargetService::TargetService(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : proto::TargetServiceBase::Component(config, context),
      repository_(
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()) {}

TargetService::CreateTargetResult TargetService::CreateTarget(
    CallContext&, proto::CreateTargetRequest&& request) {
  try {
    const auto create_request = ToDomainCreateRequest(request);
    if (const auto error =
            monitor_service::target_validator::ValidateCreateTargetRequest(
                create_request)) {
      return InvalidArgument(*error);
    }

    return MakeTargetResponse(repository_.CreateTarget(create_request));
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

TargetService::UpdateTargetResult TargetService::UpdateTarget(
    CallContext&, proto::UpdateTargetRequest&& request) {
  try {
    const auto target = ToDomainTarget(request.target());
    if (const auto error =
            monitor_service::target_validator::ValidateTarget(target)) {
      return InvalidArgument(*error);
    }

    const auto updated = repository_.UpdateTarget(target);
    if (!updated) {
      return NotFound("target not found");
    }

    return MakeTargetResponse(*updated);
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

TargetService::DeleteTargetResult TargetService::DeleteTarget(
    CallContext&, proto::TargetIdRequest&& request) {
  if (!repository_.DeactivateTarget(request.id())) {
    return NotFound("target not found");
  }

  return proto::DeleteTargetResponse{};
}

TargetService::GetTargetResult TargetService::GetTarget(
    CallContext&, proto::TargetIdRequest&& request) {
  const auto target = repository_.GetTargetById(request.id());
  if (!target) {
    return NotFound("target not found");
  }

  return MakeTargetResponse(*target);
}

TargetService::ListTargetsResult TargetService::ListTargets(
    CallContext&, proto::ListTargetsRequest&&) {
  return MakeListTargetsResponse(repository_.ListTargets());
}

TargetService::ListActiveTargetsResult TargetService::ListActiveTargets(
    CallContext&, proto::ListTargetsRequest&&) {
  return MakeListTargetsResponse(repository_.ListActiveTargets());
}

}  // namespace netwatch::target_service
