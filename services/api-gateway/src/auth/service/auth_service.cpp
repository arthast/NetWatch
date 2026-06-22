#include <auth/service/auth_service.hpp>

namespace netwatch::api_gateway::auth {

AuthService::AuthService(const auth_client::AuthClient& auth_client)
    : auth_client_(auth_client) {}

auth_client::AuthResult AuthService::Register(
    const auth_client::Credentials& credentials) const {
  return auth_client_.Register(credentials);
}

auth_client::AuthResult AuthService::Login(
    const auth_client::Credentials& credentials) const {
  return auth_client_.Login(credentials);
}

std::optional<auth_client::ValidatedSession> AuthService::ValidateToken(
    std::string_view access_token) const {
  return auth_client_.ValidateToken(access_token);
}

auth_client::ValidatedSession AuthService::VerifyEmail(
    std::string_view token) const {
  return auth_client_.VerifyEmail(token);
}

auth_client::ValidatedSession AuthService::ResendVerificationEmail(
    std::int64_t user_id) const {
  return auth_client_.ResendVerificationEmail(user_id);
}

}  // namespace netwatch::api_gateway::auth
