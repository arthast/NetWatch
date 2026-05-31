#include <common/http_response.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>

namespace netwatch::api_gateway::common {

std::string JsonResponse(const userver::server::http::HttpRequest& request,
                         const userver::formats::json::Value& body) {
  request.GetHttpResponse().SetContentType(
      userver::http::content_type::kApplicationJson);
  return userver::formats::json::ToString(body);
}

std::string ErrorResponse(const userver::server::http::HttpRequest& request,
                          userver::server::http::HttpStatus status,
                          std::string_view message) {
  userver::formats::json::ValueBuilder builder;
  builder["error"] = std::string{message};

  request.SetResponseStatus(status);
  auto response = JsonResponse(request, builder.ExtractValue());
  response.push_back('\n');
  return response;
}

}  // namespace netwatch::api_gateway::common
