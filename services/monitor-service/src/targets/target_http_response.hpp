#pragma once

#include <string>
#include <string_view>
#include <userver/formats/json/value.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

namespace monitor_service::target {
std::string JsonResponse(const userver::server::http::HttpRequest &request,
                         const userver::formats::json::Value &body);

std::string ErrorResponse(const userver::server::http::HttpRequest &request,
                          userver::server::http::HttpStatus status,
                          std::string_view message);
} // namespace monitor_service::target
