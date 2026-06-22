#include <auth/repository/auth_repository.hpp>

#include <userver/storages/postgres/row.hpp>
#include <utility>

namespace netwatch::auth_service::auth {
namespace {

User UserFromRow(const userver::storages::postgres::Row& row) {
  return User{
      .id = row["id"].As<std::int64_t>(),
      .email = row["email"].As<std::string>(),
      .created_at = row["created_at"].As<std::string>(),
      .updated_at = row["updated_at"].As<std::string>(),
      .email_verified = row["email_verified"].As<bool>(),
      .email_verified_at = row["email_verified_at"].As<std::string>(),
  };
}

constexpr std::string_view kUserFields = R"(
    id,
    email,
    to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS created_at,
    to_char(updated_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS updated_at,
    email_verified_at IS NOT NULL AS email_verified,
    COALESCE(to_char(email_verified_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"'), '') AS email_verified_at
)";

}  // namespace

AuthRepository::AuthRepository(
    userver::storages::postgres::ClusterPtr pg_cluster)
    : pg_cluster_(std::move(pg_cluster)) {}

std::optional<User> AuthRepository::GetUserById(std::int64_t user_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT " + std::string{kUserFields} +
          R"(
            FROM auth_users
            WHERE id = $1
          )",
      user_id);

  if (result.Size() == 0) {
    return std::nullopt;
  }
  return UserFromRow(result.Front());
}

std::optional<User> AuthRepository::GetUserByEmail(
    std::string_view email) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT " + std::string{kUserFields} +
          R"(
            FROM auth_users
            WHERE email = lower($1)
          )",
      email);

  if (result.Size() == 0) {
    return std::nullopt;
  }
  return UserFromRow(result.Front());
}

User AuthRepository::CreateUser(const Credentials& credentials) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO auth_users (email, password_hash) "
      "VALUES (lower($1), crypt($2, gen_salt('bf', 10))) "
      "RETURNING " +
          std::string{kUserFields},
      credentials.email, credentials.password);

  return UserFromRow(result.Front());
}

std::optional<User> AuthRepository::Authenticate(
    const Credentials& credentials) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "SELECT " + std::string{kUserFields} +
          R"(
            FROM auth_users
            WHERE email = lower($1)
              AND password_hash = crypt($2, password_hash)
          )",
      credentials.email, credentials.password);

  if (result.Size() == 0) {
    return std::nullopt;
  }
  return UserFromRow(result.Front());
}

AuthResult AuthRepository::IssueSession(const User& user) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            WITH token AS (
                SELECT encode(gen_random_bytes(32), 'hex') AS value
            ),
            session AS (
                INSERT INTO auth_sessions (user_id, token_hash, expires_at)
                SELECT
                    $1,
                    encode(digest(token.value, 'sha256'), 'hex'),
                    NOW() + INTERVAL '24 hours'
                FROM token
                RETURNING expires_at
            )
            SELECT
                token.value AS access_token,
                to_char(session.expires_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS expires_at
            FROM token
            CROSS JOIN session
          )",
      user.id);

  const auto& row = result.Front();
  return AuthResult{
      .user = user,
      .access_token = row["access_token"].As<std::string>(),
      .expires_at = row["expires_at"].As<std::string>(),
  };
}

std::optional<ValidatedSession> AuthRepository::ValidateToken(
    std::string_view access_token) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "WITH touched AS ("
      "    UPDATE auth_sessions "
      "    SET last_used_at = NOW() "
      "    WHERE token_hash = encode(digest($1, 'sha256'), 'hex') "
      "      AND expires_at > NOW() "
      "    RETURNING user_id, expires_at "
      ") "
      "SELECT " +
          std::string{kUserFields} +
          R"(,
                to_char(touched.expires_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS expires_at
            FROM touched
            JOIN auth_users ON auth_users.id = touched.user_id
          )",
      access_token);

  if (result.Size() == 0) {
    return std::nullopt;
  }

  const auto& row = result.Front();
  return ValidatedSession{
      .user = UserFromRow(row),
      .expires_at = row["expires_at"].As<std::string>(),
  };
}

EmailVerificationToken AuthRepository::CreateEmailVerificationToken(
    std::int64_t user_id) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            WITH token AS (
                SELECT encode(gen_random_bytes(32), 'hex') AS value
            ),
            created AS (
                INSERT INTO auth_email_verification_tokens (
                    user_id,
                    token_hash,
                    expires_at
                )
                SELECT
                    $1,
                    encode(digest(token.value, 'sha256'), 'hex'),
                    NOW() + INTERVAL '24 hours'
                FROM token
                RETURNING expires_at
            )
            SELECT
                token.value AS token,
                to_char(created.expires_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS expires_at
            FROM token
            CROSS JOIN created
          )",
      user_id);

  const auto& row = result.Front();
  return EmailVerificationToken{
      .token = row["token"].As<std::string>(),
      .expires_at = row["expires_at"].As<std::string>(),
  };
}

std::optional<User> AuthRepository::VerifyEmail(std::string_view token) const {
  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      R"(
            WITH token AS (
                UPDATE auth_email_verification_tokens
                SET used_at = NOW()
                WHERE token_hash = encode(digest($1, 'sha256'), 'hex')
                  AND used_at IS NULL
                  AND expires_at > NOW()
                RETURNING user_id
            ),
            verified AS (
                UPDATE auth_users
                SET
                    email_verified_at = COALESCE(email_verified_at, NOW()),
                    updated_at = NOW()
                WHERE id = (SELECT user_id FROM token)
                RETURNING *
            )
            SELECT
                id,
                email,
                to_char(created_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS created_at,
                to_char(updated_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"') AS updated_at,
                email_verified_at IS NOT NULL AS email_verified,
                COALESCE(to_char(email_verified_at AT TIME ZONE 'UTC', 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"'), '') AS email_verified_at
            FROM verified
          )",
      token);

  if (result.Size() == 0) {
    return std::nullopt;
  }
  return UserFromRow(result.Front());
}

}  // namespace netwatch::auth_service::auth
