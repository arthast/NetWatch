#pragma once

#include <string_view>

#include <auth/model/auth.hpp>
#include <auth/repository/auth_repository.hpp>

namespace netwatch::auth_service::auth {

class AuthService {
 public:
  explicit AuthService(AuthRepository repository);

  AuthResult Register(const Credentials& credentials) const;

  AuthResult Login(const Credentials& credentials) const;

  ValidatedSession ValidateToken(std::string_view access_token) const;

 private:
  AuthRepository repository_;
};

}  // namespace netwatch::auth_service::auth
