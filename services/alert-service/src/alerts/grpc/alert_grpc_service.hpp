#pragma once

#include <string_view>

#include <netwatch/monitor_service_service.usrv.pb.hpp>
#include <userver/components/component.hpp>

#include <alerts/storage/alert_repository.hpp>
#include <alerts/service/alert_service.hpp>

namespace monitor_service::alerts {

class AlertGrpcService final
    : public netwatch::monitor::v1::AlertServiceBase::Component {
 public:
  static constexpr std::string_view kName = "alert-grpc-service";

  AlertGrpcService(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

  ListAlertsResult ListAlerts(
      CallContext& context,
      netwatch::monitor::v1::ListAlertsRequest&& request) override;

  ListActiveAlertsResult ListActiveAlerts(
      CallContext& context,
      netwatch::monitor::v1::ListAlertsRequest&& request) override;

  ProcessCheckResultResult ProcessCheckResult(
      CallContext& context,
      netwatch::monitor::v1::ProcessCheckResultRequest&& request) override;

 private:
  AlertRepository repository_;
  AlertService alert_service_;
};

}  // namespace monitor_service::alerts
