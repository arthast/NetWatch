#include <auth_client/client/auth_client.hpp>

#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/simple_client_component.hpp>

#include <client_common/call_options.hpp>

namespace netwatch::auth_client {
namespace {

namespace proto = netwatch::auth::v1;

User ToDomainUser(const proto::User& user) {
  return User{
      .id = user.id(),
      .email = user.email(),
      .created_at = user.created_at(),
      .updated_at = user.updated_at(),
      .email_verified = user.email_verified(),
      .email_verified_at = user.email_verified_at(),
  };
}

AuthResult ToDomainAuthResult(const proto::AuthResponse& response) {
  return AuthResult{
      .user = ToDomainUser(response.user()),
      .access_token = response.access_token(),
      .expires_at = response.expires_at(),
  };
}

ValidatedSession ToDomainSession(const proto::ValidateTokenResponse& response) {
  return ValidatedSession{
      .user = ToDomainUser(response.user()),
      .expires_at = response.expires_at(),
  };
}

proto::RegisterRequest MakeRegisterRequest(const Credentials& credentials) {
  proto::RegisterRequest request;
  request.set_email(credentials.email);
  request.set_password(credentials.password);
  return request;
}

proto::LoginRequest MakeLoginRequest(const Credentials& credentials) {
  proto::LoginRequest request;
  request.set_email(credentials.email);
  request.set_password(credentials.password);
  return request;
}

proto::ValidateTokenRequest MakeValidateTokenRequest(
    std::string_view access_token) {
  proto::ValidateTokenRequest request;
  request.set_access_token(std::string{access_token});
  return request;
}

proto::VerifyEmailRequest MakeVerifyEmailRequest(std::string_view token) {
  proto::VerifyEmailRequest request;
  request.set_token(std::string{token});
  return request;
}

proto::ResendVerificationEmailRequest MakeResendVerificationEmailRequest(
    std::int64_t user_id) {
  proto::ResendVerificationEmailRequest request;
  request.set_user_id(user_id);
  return request;
}

std::invalid_argument ToInvalidArgument(
    const userver::ugrpc::client::InvalidArgumentError& ex) {
  return std::invalid_argument{ex.GetStatus().error_message()};
}

}  // namespace

AuthClient::AuthClient(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      grpc_client_(
          &context
               .FindComponent<userver::ugrpc::client::SimpleClientComponent<
                   proto::AuthServiceClient>>("auth-service-client")
               .GetClient()) {}

AuthResult AuthClient::Register(const Credentials& credentials) const {
  try {
    return ToDomainAuthResult(grpc_client_->Register(
        MakeRegisterRequest(credentials), client_common::MakeGrpcCallOptions()));
  } catch (const userver::ugrpc::client::InvalidArgumentError& ex) {
    throw ToInvalidArgument(ex);
  } catch (const userver::ugrpc::client::AlreadyExistsError& ex) {
    throw std::invalid_argument{ex.GetStatus().error_message()};
  }
}

AuthResult AuthClient::Login(const Credentials& credentials) const {
  try {
    return ToDomainAuthResult(grpc_client_->Login(
        MakeLoginRequest(credentials), client_common::MakeGrpcCallOptions()));
  } catch (const userver::ugrpc::client::InvalidArgumentError& ex) {
    throw ToInvalidArgument(ex);
  }
}

std::optional<ValidatedSession> AuthClient::ValidateToken(
    std::string_view access_token) const {
  try {
    return ToDomainSession(grpc_client_->ValidateToken(
        MakeValidateTokenRequest(access_token),
        client_common::MakeGrpcCallOptions()));
  } catch (const userver::ugrpc::client::UnauthenticatedError&) {
    return std::nullopt;
  }
}

ValidatedSession AuthClient::VerifyEmail(std::string_view token) const {
  try {
    return ToDomainSession(grpc_client_->VerifyEmail(
        MakeVerifyEmailRequest(token), client_common::MakeGrpcCallOptions()));
  } catch (const userver::ugrpc::client::InvalidArgumentError& ex) {
    throw ToInvalidArgument(ex);
  }
}

ValidatedSession AuthClient::ResendVerificationEmail(
    std::int64_t user_id) const {
  try {
    return ToDomainSession(grpc_client_->ResendVerificationEmail(
        MakeResendVerificationEmailRequest(user_id),
        client_common::MakeGrpcCallOptions()));
  } catch (const userver::ugrpc::client::InvalidArgumentError& ex) {
    throw ToInvalidArgument(ex);
  }
}

}  // namespace netwatch::auth_client
