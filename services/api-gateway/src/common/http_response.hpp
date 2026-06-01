#pragma once

#include <string>
#include <string_view>
#include <userver/formats/json/value.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/ugrpc/client/exceptions.hpp>

namespace netwatch::api_gateway::common {

std::string JsonResponse(const userver::server::http::HttpRequest& request,
                         const userver::formats::json::Value& body);

std::string ErrorResponse(const userver::server::http::HttpRequest& request,
                          userver::server::http::HttpStatus status,
                          std::string_view message);

std::string UpstreamErrorResponse(
    const userver::server::http::HttpRequest& request,
    std::string_view upstream_service,
    const userver::ugrpc::client::BaseError& error);

}  // namespace netwatch::api_gateway::common
