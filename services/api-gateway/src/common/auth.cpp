#include <common/auth.hpp>

#include <optional>
#include <string_view>

namespace netwatch::api_gateway::common {
namespace {

constexpr std::string_view kBearerPrefix = "Bearer ";

std::optional<std::string_view> ExtractBearerToken(
    const userver::server::http::HttpRequest& request) {
  const auto header = request.GetHeader("Authorization");
  if (header.size() <= kBearerPrefix.size() ||
      header.substr(0, kBearerPrefix.size()) != kBearerPrefix) {
    return std::nullopt;
  }
  return header.substr(kBearerPrefix.size());
}

}  // namespace

std::optional<auth_client::ValidatedSession> AuthenticateRequest(
    const userver::server::http::HttpRequest& request,
    const auth::AuthService& auth_service) {
  const auto access_token = ExtractBearerToken(request);
  if (!access_token) {
    return std::nullopt;
  }

  return auth_service.ValidateToken(*access_token);
}

}  // namespace netwatch::api_gateway::common
