#include <auth/grpc/auth_grpc_service.hpp>

#include <grpcpp/support/status.h>
#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/component.hpp>

namespace netwatch::auth_service::auth {
namespace {

namespace proto = netwatch::auth::v1;

grpc::Status InvalidArgument(std::string message) {
  return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, std::move(message)};
}

grpc::Status AlreadyExists(std::string message) {
  return grpc::Status{grpc::StatusCode::ALREADY_EXISTS, std::move(message)};
}

grpc::Status Unauthenticated(std::string message) {
  return grpc::Status{grpc::StatusCode::UNAUTHENTICATED, std::move(message)};
}

Credentials ToCredentials(std::string email, std::string password) {
  return Credentials{
      .email = std::move(email),
      .password = std::move(password),
  };
}

void FillProtoUser(const User& source, proto::User& target) {
  target.set_id(source.id);
  target.set_email(source.email);
  target.set_created_at(source.created_at);
  target.set_updated_at(source.updated_at);
}

proto::AuthResponse MakeAuthResponse(const AuthResult& result) {
  proto::AuthResponse response;
  FillProtoUser(result.user, *response.mutable_user());
  response.set_access_token(result.access_token);
  response.set_expires_at(result.expires_at);
  return response;
}

proto::ValidateTokenResponse MakeValidateTokenResponse(
    const ValidatedSession& session) {
  proto::ValidateTokenResponse response;
  FillProtoUser(session.user, *response.mutable_user());
  response.set_expires_at(session.expires_at);
  return response;
}

}  // namespace

AuthGrpcService::AuthGrpcService(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : proto::AuthServiceBase::Component(config, context),
      auth_service_(AuthRepository{
          context.FindComponent<userver::components::Postgres>("postgres-db-1")
              .GetCluster()}) {}

AuthGrpcService::RegisterResult AuthGrpcService::Register(
    CallContext&, proto::RegisterRequest&& request) {
  try {
    return MakeAuthResponse(auth_service_.Register(
        ToCredentials(request.email(), request.password())));
  } catch (const DuplicateEmail& ex) {
    return AlreadyExists(ex.what());
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

AuthGrpcService::LoginResult AuthGrpcService::Login(
    CallContext&, proto::LoginRequest&& request) {
  try {
    return MakeAuthResponse(
        auth_service_.Login(ToCredentials(request.email(), request.password())));
  } catch (const InvalidCredentials& ex) {
    return Unauthenticated(ex.what());
  } catch (const std::invalid_argument& ex) {
    return InvalidArgument(ex.what());
  }
}

AuthGrpcService::ValidateTokenResult AuthGrpcService::ValidateToken(
    CallContext&, proto::ValidateTokenRequest&& request) {
  try {
    return MakeValidateTokenResponse(
        auth_service_.ValidateToken(request.access_token()));
  } catch (const InvalidToken& ex) {
    return Unauthenticated(ex.what());
  }
}

}  // namespace netwatch::auth_service::auth
