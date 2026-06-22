#include <auth/json/auth_json.hpp>

#include <stdexcept>
#include <string>
#include <string_view>
#include <userver/formats/json/value_builder.hpp>

namespace netwatch::api_gateway::auth {
namespace {

template <typename T>
T ReadRequired(const userver::formats::json::Value& json,
               std::string_view field) {
  const auto value = json[field];
  if (value.IsMissing() || value.IsNull()) {
    throw std::invalid_argument{std::string{field} + " is required"};
  }
  return value.As<T>();
}

}  // namespace

auth_client::Credentials ParseCredentials(
    const userver::formats::json::Value& json) {
  if (!json.IsObject()) {
    throw std::invalid_argument{"request body must be a JSON object"};
  }

  return auth_client::Credentials{
      .email = ReadRequired<std::string>(json, "email"),
      .password = ReadRequired<std::string>(json, "password"),
  };
}

userver::formats::json::Value SerializeUser(const auth_client::User& user) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = user.id;
  builder["email"] = user.email;
  builder["created_at"] = user.created_at;
  builder["updated_at"] = user.updated_at;
  builder["email_verified"] = user.email_verified;
  builder["email_verified_at"] = user.email_verified_at;
  return builder.ExtractValue();
}

userver::formats::json::Value SerializeAuthResult(
    const auth_client::AuthResult& result) {
  userver::formats::json::ValueBuilder builder;
  builder["user"] = SerializeUser(result.user);
  builder["access_token"] = result.access_token;
  builder["expires_at"] = result.expires_at;
  return builder.ExtractValue();
}

userver::formats::json::Value SerializeValidatedSession(
    const auth_client::ValidatedSession& session) {
  userver::formats::json::ValueBuilder builder;
  builder["user"] = SerializeUser(session.user);
  builder["expires_at"] = session.expires_at;
  return builder.ExtractValue();
}

}  // namespace netwatch::api_gateway::auth
