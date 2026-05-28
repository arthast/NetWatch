#include <targets/client/target_client.hpp>

#include <chrono>
#include <optional>
#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/ugrpc/client/call_options.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/simple_client_component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

#include <targets/validation/target_validator.hpp>

namespace monitor_service::target {
namespace {

namespace proto = netwatch::target::v1;

userver::ugrpc::client::CallOptions MakeCallOptions() {
  userver::ugrpc::client::CallOptions options;
  options.SetAttempts(1);
  options.SetTimeout(std::chrono::milliseconds{1000});
  return options;
}

proto::TargetType ToProtoTargetType(TargetType type) {
  switch (type) {
    case TargetType::kHttp:
      return proto::TARGET_TYPE_HTTP;
    case TargetType::kTcp:
      return proto::TARGET_TYPE_TCP;
  }
  return proto::TARGET_TYPE_UNSPECIFIED;
}

TargetType ToDomainTargetType(proto::TargetType type) {
  switch (type) {
    case proto::TARGET_TYPE_HTTP:
      return TargetType::kHttp;
    case proto::TARGET_TYPE_TCP:
      return TargetType::kTcp;
    case proto::TARGET_TYPE_UNSPECIFIED:
    case proto::TargetType_INT_MIN_SENTINEL_DO_NOT_USE_:
    case proto::TargetType_INT_MAX_SENTINEL_DO_NOT_USE_:
      break;
  }

  throw std::invalid_argument("target type must be http or tcp");
}

void FillProtoTarget(const Target& source, proto::Target& target) {
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

proto::CreateTargetRequest ToProtoCreateRequest(
    const CreateTargetRequest& request) {
  proto::CreateTargetRequest result;
  result.set_name(request.name);
  result.set_type(ToProtoTargetType(request.type));

  if (request.url) {
    result.set_url(*request.url);
  }
  if (request.method) {
    result.set_method(*request.method);
  }
  if (request.expected_status_code) {
    result.set_expected_status_code(*request.expected_status_code);
  }
  if (request.host) {
    result.set_host(*request.host);
  }
  if (request.port) {
    result.set_port(*request.port);
  }

  result.set_interval_seconds(request.interval_seconds);
  result.set_timeout_ms(request.timeout_ms);
  return result;
}

proto::TargetIdRequest MakeTargetIdRequest(std::int64_t target_id) {
  proto::TargetIdRequest request;
  request.set_id(target_id);
  return request;
}

Target ToDomainTarget(const proto::Target& target) {
  return Target{
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

std::vector<Target> ToDomainTargets(
    const proto::ListTargetsResponse& response) {
  std::vector<Target> targets;
  targets.reserve(response.targets_size());
  for (const auto& target : response.targets()) {
    targets.push_back(ToDomainTarget(target));
  }
  return targets;
}

std::invalid_argument ToInvalidArgument(
    const userver::ugrpc::client::InvalidArgumentError& ex) {
  return std::invalid_argument{ex.GetStatus().error_message()};
}

bool ShouldUseGrpc(const userver::components::ComponentConfig& config) {
  const auto transport = config["transport"].As<std::string>("grpc");
  if (transport == "grpc") {
    return true;
  }
  if (transport == "postgres") {
    return false;
  }

  throw std::invalid_argument{
      "target-client.transport must be either 'grpc' or 'postgres'"};
}

}  // namespace

TargetClient::TargetClient(const userver::components::ComponentConfig& config,
                           const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      use_grpc_(ShouldUseGrpc(config)),
      grpc_client_(use_grpc_
                       ? &context
                              .FindComponent<
                                  userver::ugrpc::client::SimpleClientComponent<
                                      proto::TargetServiceClient>>(
                                  "target-service-client")
                              .GetClient()
                       : nullptr),
      repository_(use_grpc_
                      ? std::nullopt
                      : std::make_optional(TargetRepository{
                            context
                                .FindComponent<userver::components::Postgres>(
                                    "postgres-db-1")
                                .GetCluster()})) {}

userver::yaml_config::Schema TargetClient::GetStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(
      R"(
type: object
description: target storage client
additionalProperties: false
properties:
    transport:
        type: string
        description: target-service transport; use grpc in production and postgres in single-service tests
        defaultDescription: grpc
)");
}

Target TargetClient::CreateTarget(const CreateTargetRequest& request) const {
  if (!use_grpc_) {
    if (const auto error =
            target_validator::ValidateCreateTargetRequest(request)) {
      throw std::invalid_argument{*error};
    }
    return repository_->CreateTarget(request);
  }

  try {
    return ToDomainTarget(
        grpc_client_
            ->CreateTarget(ToProtoCreateRequest(request), MakeCallOptions())
            .target());
  } catch (const userver::ugrpc::client::InvalidArgumentError& ex) {
    throw ToInvalidArgument(ex);
  }
}

std::vector<Target> TargetClient::ListActiveTargets() const {
  if (!use_grpc_) {
    return repository_->ListActiveTargets();
  }

  return ToDomainTargets(grpc_client_->ListActiveTargets(
      proto::ListTargetsRequest{}, MakeCallOptions()));
}

std::optional<Target> TargetClient::GetTargetById(
    std::int64_t target_id) const {
  if (!use_grpc_) {
    return repository_->GetTargetById(target_id);
  }

  try {
    return ToDomainTarget(
        grpc_client_
            ->GetTarget(MakeTargetIdRequest(target_id), MakeCallOptions())
            .target());
  } catch (const userver::ugrpc::client::NotFoundError&) {
    return std::nullopt;
  }
}

std::optional<Target> TargetClient::UpdateTarget(const Target& target) const {
  if (!use_grpc_) {
    if (const auto error = target_validator::ValidateTarget(target)) {
      throw std::invalid_argument{*error};
    }
    return repository_->UpdateTarget(target);
  }

  proto::UpdateTargetRequest request;
  FillProtoTarget(target, *request.mutable_target());

  try {
    return ToDomainTarget(
        grpc_client_->UpdateTarget(request, MakeCallOptions()).target());
  } catch (const userver::ugrpc::client::NotFoundError&) {
    return std::nullopt;
  } catch (const userver::ugrpc::client::InvalidArgumentError& ex) {
    throw ToInvalidArgument(ex);
  }
}

bool TargetClient::DeactivateTarget(std::int64_t target_id) const {
  if (!use_grpc_) {
    return repository_->DeactivateTarget(target_id);
  }

  try {
    grpc_client_->DeleteTarget(MakeTargetIdRequest(target_id),
                               MakeCallOptions());
    return true;
  } catch (const userver::ugrpc::client::NotFoundError&) {
    return false;
  }
}

}  // namespace monitor_service::target
