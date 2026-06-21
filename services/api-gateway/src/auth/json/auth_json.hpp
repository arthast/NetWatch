#pragma once

#include <userver/formats/json/value.hpp>

#include <auth_client/model/auth.hpp>

namespace netwatch::api_gateway::auth {

auth_client::Credentials ParseCredentials(
    const userver::formats::json::Value& json);

userver::formats::json::Value SerializeUser(const auth_client::User& user);

userver::formats::json::Value SerializeAuthResult(
    const auth_client::AuthResult& result);

userver::formats::json::Value SerializeValidatedSession(
    const auth_client::ValidatedSession& session);

}  // namespace netwatch::api_gateway::auth
