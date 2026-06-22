#pragma once

#include <string_view>

#include <auth/email/email_verification_sender.hpp>
#include <auth/model/auth.hpp>
#include <auth/repository/auth_repository.hpp>

namespace netwatch::auth_service::auth {

class AuthService {
 public:
  AuthService(AuthRepository repository,
              const EmailVerificationSender& email_verification_sender);

  AuthResult Register(const Credentials& credentials) const;

  AuthResult Login(const Credentials& credentials) const;

  ValidatedSession ValidateToken(std::string_view access_token) const;

  ValidatedSession VerifyEmail(std::string_view token) const;

  ValidatedSession ResendVerificationEmail(std::int64_t user_id) const;

 private:
  void SendVerificationEmail(const User& user) const;

  AuthRepository repository_;
  const EmailVerificationSender& email_verification_sender_;
};

}  // namespace netwatch::auth_service::auth
