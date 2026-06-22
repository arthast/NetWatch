#pragma once

#include <optional>
#include <string_view>

#include <auth_client/client/auth_client.hpp>
#include <auth_client/model/auth.hpp>

namespace netwatch::api_gateway::auth {

class AuthService {
 public:
  explicit AuthService(const auth_client::AuthClient& auth_client);

  auth_client::AuthResult Register(
      const auth_client::Credentials& credentials) const;

  auth_client::AuthResult Login(
      const auth_client::Credentials& credentials) const;

  std::optional<auth_client::ValidatedSession> ValidateToken(
      std::string_view access_token) const;

  auth_client::ValidatedSession VerifyEmail(std::string_view token) const;

  auth_client::ValidatedSession ResendVerificationEmail(
      std::int64_t user_id) const;

 private:
  const auth_client::AuthClient& auth_client_;
};

}  // namespace netwatch::api_gateway::auth
