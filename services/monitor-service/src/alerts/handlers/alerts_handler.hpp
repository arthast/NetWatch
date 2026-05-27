#pragma once

#include <string>
#include <string_view>
#include <userver/server/handlers/http_handler_base.hpp>

#include <alerts/storage/alert_repository.hpp>

namespace monitor_service::alerts {

class AlertsHandler final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-alerts";

  AlertsHandler(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& component_context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

 private:
  AlertRepository repository_;
};

}  // namespace monitor_service::alerts
