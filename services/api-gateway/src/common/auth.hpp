#pragma once

#include <optional>

#include <auth/service/auth_service.hpp>
#include <auth_client/model/auth.hpp>
#include <userver/server/http/http_request.hpp>

namespace netwatch::api_gateway::common {

std::optional<auth_client::ValidatedSession> AuthenticateRequest(
    const userver::server::http::HttpRequest& request,
    const auth::AuthService& auth_service);

}  // namespace netwatch::api_gateway::common
