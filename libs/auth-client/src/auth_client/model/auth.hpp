#pragma once

#include <cstdint>
#include <string>

namespace netwatch::auth_client {

struct User {
  std::int64_t id;
  std::string email;
  std::string created_at;
  std::string updated_at;
};

struct Credentials {
  std::string email;
  std::string password;
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

}  // namespace netwatch::auth_client
