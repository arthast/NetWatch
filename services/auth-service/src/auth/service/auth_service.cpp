#include <auth/service/auth_service.hpp>

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

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

AuthService::AuthService(AuthRepository repository)
    : repository_(std::move(repository)) {}

AuthResult AuthService::Register(const Credentials& credentials) const {
  ValidateCredentials(credentials);

  if (repository_.GetUserByEmail(credentials.email)) {
    throw DuplicateEmail{};
  }

  return repository_.IssueSession(repository_.CreateUser(credentials));
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

}  // namespace netwatch::auth_service::auth
