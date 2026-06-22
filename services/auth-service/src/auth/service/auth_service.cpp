#include <auth/service/auth_service.hpp>

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>
#include <userver/logging/log.hpp>

namespace netwatch::auth_service::auth {
namespace {

bool IsBlank(std::string_view value) {
  return value.find_first_not_of(" \t\n\r") == std::string_view::npos;
}

bool IsValidEmail(std::string_view email) {
  if (email.size() < 3 || email.size() > 320) {
    return false;
  }

  const auto at = email.find('@');
  if (at == std::string_view::npos || at == 0 || at + 1 >= email.size()) {
    return false;
  }
  if (email.find('@', at + 1) != std::string_view::npos) {
    return false;
  }
  if (email.find('.', at + 1) == std::string_view::npos) {
    return false;
  }

  return std::none_of(email.begin(), email.end(), [](unsigned char ch) {
    return std::isspace(ch) != 0 || std::iscntrl(ch) != 0;
  });
}

void ValidateCredentials(const Credentials& credentials) {
  if (!IsValidEmail(credentials.email)) {
    throw std::invalid_argument{"email must be a valid address"};
  }
  if (credentials.password.size() < 8) {
    throw std::invalid_argument{"password must contain at least 8 characters"};
  }
  if (IsBlank(credentials.password)) {
    throw std::invalid_argument{"password must not be blank"};
  }
}

}  // namespace

DuplicateEmail::DuplicateEmail()
    : std::runtime_error{"user with this email already exists"} {}

InvalidCredentials::InvalidCredentials()
    : std::runtime_error{"invalid email or password"} {}

InvalidToken::InvalidToken() : std::runtime_error{"access token is invalid"} {}

InvalidVerificationToken::InvalidVerificationToken()
    : std::runtime_error{"email verification token is invalid or expired"} {}

AuthService::AuthService(
    AuthRepository repository,
    const EmailVerificationSender& email_verification_sender)
    : repository_(std::move(repository)),
      email_verification_sender_(email_verification_sender) {}

AuthResult AuthService::Register(const Credentials& credentials) const {
  ValidateCredentials(credentials);

  if (repository_.GetUserByEmail(credentials.email)) {
    throw DuplicateEmail{};
  }

  auto user = repository_.CreateUser(credentials);
  SendVerificationEmail(user);
  return repository_.IssueSession(user);
}

AuthResult AuthService::Login(const Credentials& credentials) const {
  ValidateCredentials(credentials);

  const auto user = repository_.Authenticate(credentials);
  if (!user) {
    throw InvalidCredentials{};
  }

  return repository_.IssueSession(*user);
}

ValidatedSession AuthService::ValidateToken(
    std::string_view access_token) const {
  if (access_token.empty() || access_token.size() > 512) {
    throw InvalidToken{};
  }

  const auto session = repository_.ValidateToken(access_token);
  if (!session) {
    throw InvalidToken{};
  }

  return *session;
}

ValidatedSession AuthService::VerifyEmail(std::string_view token) const {
  if (token.empty() || token.size() > 512) {
    throw InvalidVerificationToken{};
  }

  const auto user = repository_.VerifyEmail(token);
  if (!user) {
    throw InvalidVerificationToken{};
  }

  return ValidatedSession{
      .user = *user,
      .expires_at = {},
  };
}

ValidatedSession AuthService::ResendVerificationEmail(
    std::int64_t user_id) const {
  if (user_id <= 0) {
    throw std::invalid_argument{"user id must be a positive integer"};
  }

  const auto user = repository_.GetUserById(user_id);
  if (!user) {
    throw InvalidToken{};
  }

  if (!user->email_verified) {
    SendVerificationEmail(*user);
  }

  return ValidatedSession{
      .user = *user,
      .expires_at = {},
  };
}

void AuthService::SendVerificationEmail(const User& user) const {
  try {
    const auto verification = repository_.CreateEmailVerificationToken(user.id);
    email_verification_sender_.SendVerificationEmail(user.email,
                                                     verification.token);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Failed to send email verification message, user_id="
                  << user.id << ", email=" << user.email
                  << ", error=" << ex.what();
  }
}

}  // namespace netwatch::auth_service::auth
