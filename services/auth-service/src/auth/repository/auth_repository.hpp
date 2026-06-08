#pragma once

#include <optional>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/postgres.hpp>

#include <auth/model/auth.hpp>

namespace netwatch::auth_service::auth {

class AuthRepository {
 public:
  explicit AuthRepository(userver::storages::postgres::ClusterPtr pg_cluster);

  std::optional<User> GetUserByEmail(std::string_view email) const;

  User CreateUser(const Credentials& credentials) const;

  std::optional<User> Authenticate(const Credentials& credentials) const;

  AuthResult IssueSession(const User& user) const;

  std::optional<ValidatedSession> ValidateToken(
      std::string_view access_token) const;

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace netwatch::auth_service::auth
