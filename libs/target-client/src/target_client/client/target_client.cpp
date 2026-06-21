#include <target_client/client/target_client.hpp>

#include <optional>
#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/simple_client_component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

#include <client_common/call_options.hpp>

namespace netwatch::target_client {
namespace {

namespace proto = netwatch::target::v1;

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

void FillProtoUpdateRequest(std::int64_t target_id,
                            const UpdateTargetRequest& source,
                            proto::UpdateTargetRequest& request) {
  request.set_id(target_id);

  if (source.name) {
    request.set_name(*source.name);
  }
  if (source.type) {
    request.set_type(ToProtoTargetType(*source.type));
  }
  if (source.url) {
    request.set_url(*source.url);
  }
  if (source.method) {
    request.set_method(*source.method);
  }
  if (source.expected_status_code) {
    request.set_expected_status_code(*source.expected_status_code);
  }
  if (source.host) {
    request.set_host(*source.host);
  }
  if (source.port) {
    request.set_port(*source.port);
  }
  if (source.interval_seconds) {
    request.set_interval_seconds(*source.interval_seconds);
  }
  if (source.timeout_ms) {
    request.set_timeout_ms(*source.timeout_ms);
  }
  if (source.user_id) {
    request.set_user_id(*source.user_id);
  }
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
  if (request.user_id) {
    result.set_user_id(*request.user_id);
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

proto::TargetIdRequest MakeTargetIdRequest(std::int64_t target_id,
                                           std::int64_t user_id) {
  auto request = MakeTargetIdRequest(target_id);
  request.set_user_id(user_id);
  return request;
}

proto::ListTargetsRequest MakeListTargetsForUserRequest(std::int64_t user_id) {
  proto::ListTargetsRequest request;
  request.set_user_id(user_id);
  return request;
}

Target ToDomainTarget(const proto::Target& target) {
  return Target{
      .id = target.id(),
      .user_id = target.has_user_id() ? std::make_optional(target.user_id())
                                      : std::nullopt,
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

}  // namespace

TargetClient::TargetClient(const userver::components::ComponentConfig& config,
                           const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      grpc_client_(
          &context
               .FindComponent<userver::ugrpc::client::SimpleClientComponent<
                   proto::TargetServiceClient>>("target-service-client")
               .GetClient()) {
  const auto transport = config["transport"].As<std::optional<std::string>>();
  if (transport && *transport != "grpc") {
    throw std::invalid_argument{"target-client.transport supports only 'grpc'"};
  }
}

userver::yaml_config::Schema TargetClient::GetStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(
      R"(
type: object
description: target-service grpc client
additionalProperties: false
properties:
    transport:
        type: string
        description: deprecated target-service transport option; only grpc is supported
        defaultDescription: grpc
)");
}

Target TargetClient::CreateTarget(const CreateTargetRequest& request) const {
  try {
    return ToDomainTarget(
        grpc_client_
            ->CreateTarget(ToProtoCreateRequest(request),
                           client_common::MakeGrpcCallOptions())
            .target());
  } catch (const userver::ugrpc::client::InvalidArgumentError& ex) {
    throw ToInvalidArgument(ex);
  }
}

std::vector<Target> TargetClient::ListActiveTargets() const {
  return ToDomainTargets(grpc_client_->ListActiveTargets(
      proto::ListTargetsRequest{}, client_common::MakeGrpcCallOptions()));
}

std::vector<Target> TargetClient::ListActiveTargetsForUser(
    std::int64_t user_id) const {
  return ToDomainTargets(
      grpc_client_->ListActiveTargets(MakeListTargetsForUserRequest(user_id),
                                      client_common::MakeGrpcCallOptions()));
}

std::optional<Target> TargetClient::GetTargetById(
    std::int64_t target_id) const {
  try {
    return ToDomainTarget(grpc_client_
                              ->GetTarget(MakeTargetIdRequest(target_id),
                                          client_common::MakeGrpcCallOptions())
                              .target());
  } catch (const userver::ugrpc::client::NotFoundError&) {
    return std::nullopt;
  }
}

std::optional<Target> TargetClient::GetTargetByIdForUser(
    std::int64_t target_id, std::int64_t user_id) const {
  try {
    return ToDomainTarget(
        grpc_client_
            ->GetTarget(MakeTargetIdRequest(target_id, user_id),
                        client_common::MakeGrpcCallOptions())
            .target());
  } catch (const userver::ugrpc::client::NotFoundError&) {
    return std::nullopt;
  }
}

std::optional<Target> TargetClient::UpdateTarget(
    std::int64_t target_id, const UpdateTargetRequest& update) const {
  proto::UpdateTargetRequest request;
  FillProtoUpdateRequest(target_id, update, request);

  try {
    return ToDomainTarget(
        grpc_client_
            ->UpdateTarget(request, client_common::MakeGrpcCallOptions())
            .target());
  } catch (const userver::ugrpc::client::NotFoundError&) {
    return std::nullopt;
  } catch (const userver::ugrpc::client::InvalidArgumentError& ex) {
    throw ToInvalidArgument(ex);
  }
}

std::optional<Target> TargetClient::UpdateTargetForUser(
    std::int64_t target_id, std::int64_t user_id,
    const UpdateTargetRequest& update) const {
  auto scoped_update = update;
  scoped_update.user_id = user_id;
  return UpdateTarget(target_id, scoped_update);
}

bool TargetClient::DeactivateTarget(std::int64_t target_id) const {
  try {
    grpc_client_->DeleteTarget(MakeTargetIdRequest(target_id),
                               client_common::MakeGrpcCallOptions());
    return true;
  } catch (const userver::ugrpc::client::NotFoundError&) {
    return false;
  }
}

bool TargetClient::DeactivateTargetForUser(std::int64_t target_id,
                                           std::int64_t user_id) const {
  try {
    grpc_client_->DeleteTarget(MakeTargetIdRequest(target_id, user_id),
                               client_common::MakeGrpcCallOptions());
    return true;
  } catch (const userver::ugrpc::client::NotFoundError&) {
    return false;
  }
}

}  // namespace netwatch::target_client
