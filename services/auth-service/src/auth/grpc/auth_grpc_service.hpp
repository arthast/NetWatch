#pragma once

#include <string_view>
#include <userver/components/component.hpp>

#include <auth/service/auth_service.hpp>
#include <netwatch/auth_service_service.usrv.pb.hpp>

namespace netwatch::auth_service::auth {

class AuthGrpcService final : public netwatch::auth::v1::AuthServiceBase::Component {
 public:
  static constexpr std::string_view kName = "auth-grpc-service";

  AuthGrpcService(const userver::components::ComponentConfig& config,
                  const userver::components::ComponentContext& context);

  RegisterResult Register(
      CallContext& context, netwatch::auth::v1::RegisterRequest&& request) override;

  LoginResult Login(CallContext& context,
                    netwatch::auth::v1::LoginRequest&& request) override;

  ValidateTokenResult ValidateToken(
      CallContext& context,
      netwatch::auth::v1::ValidateTokenRequest&& request) override;

 private:
  AuthService auth_service_;
};

}  // namespace netwatch::auth_service::auth
