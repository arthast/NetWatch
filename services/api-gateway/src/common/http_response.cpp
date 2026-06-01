#include <common/http_response.hpp>

#include <typeinfo>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>
#include <userver/logging/log.hpp>

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

std::string UpstreamErrorResponse(
    const userver::server::http::HttpRequest& request,
    std::string_view upstream_service,
    const userver::ugrpc::client::BaseError& error) {
  LOG_WARNING() << "Upstream gRPC call failed, service=" << upstream_service
                << ", error_type=" << typeid(error).name()
                << ", error=" << error.what();

  if (dynamic_cast<const userver::ugrpc::client::DeadlineExceededError*>(
          &error) != nullptr) {
    return ErrorResponse(
        request, userver::server::http::HttpStatus::kGatewayTimeout,
        std::string{upstream_service} + " timed out");
  }

  return ErrorResponse(request,
                       userver::server::http::HttpStatus::kBadGateway,
                       std::string{upstream_service} + " is unavailable");
}

}  // namespace netwatch::api_gateway::common
