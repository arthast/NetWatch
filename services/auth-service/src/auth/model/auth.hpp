#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace netwatch::auth_service::auth {

struct User {
  std::int64_t id;
  std::string email;
  std::string created_at;
  std::string updated_at;
};

struct AuthResult {
  User user;
  std::string access_token;
  std::string expires_at;
};

struct ValidatedSession {
  User user;
  std::string expires_at;
};

struct Credentials {
  std::string email;
  std::string password;
};

class DuplicateEmail final : public std::runtime_error {
 public:
  DuplicateEmail();
};

class InvalidCredentials final : public std::runtime_error {
 public:
  InvalidCredentials();
};

class InvalidToken final : public std::runtime_error {
 public:
  InvalidToken();
};

}  // namespace netwatch::auth_service::auth
