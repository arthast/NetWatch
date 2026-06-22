#pragma once

#include <optional>
#include <string_view>
#include <userver/components/component_base.hpp>

#include <auth_client/model/auth.hpp>
#include <netwatch/auth_service_client.usrv.pb.hpp>

namespace netwatch::auth_client {

class AuthClient final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "auth-client";

  AuthClient(const userver::components::ComponentConfig& config,
             const userver::components::ComponentContext& context);

  AuthResult Register(const Credentials& credentials) const;

  AuthResult Login(const Credentials& credentials) const;

  std::optional<ValidatedSession> ValidateToken(
      std::string_view access_token) const;

  ValidatedSession VerifyEmail(std::string_view token) const;

  ValidatedSession ResendVerificationEmail(std::int64_t user_id) const;

 private:
  netwatch::auth::v1::AuthServiceClient* grpc_client_;
};

}  // namespace netwatch::auth_client
