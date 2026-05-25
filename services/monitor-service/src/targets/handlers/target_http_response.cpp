#include <targets/handlers/target_http_response.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/http/content_type.hpp>

#include <targets/json/target_json.hpp>

namespace monitor_service::target {
std::string JsonResponse(const userver::server::http::HttpRequest &request,
                         const userver::formats::json::Value &body) {
    request.GetHttpResponse().SetContentType(
        userver::http::content_type::kApplicationJson);
    return userver::formats::json::ToString(body);
}

std::string ErrorResponse(const userver::server::http::HttpRequest &request,
                          userver::server::http::HttpStatus status,
                          std::string_view message) {
    request.SetResponseStatus(status);
    auto response = JsonResponse(request, SerializeError(message));
    response.push_back('\n');
    return response;
}
} // namespace monitor_service::target
