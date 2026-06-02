#pragma once

#include <string_view>

#include <netwatch/monitor_service_service.usrv.pb.hpp>
#include <userver/components/component.hpp>

#include <checks/service/check_service.hpp>

namespace netwatch::monitor_service::checks {

class CheckGrpcService final
    : public netwatch::monitor::v1::CheckServiceBase::Component {
 public:
  static constexpr std::string_view kName = "check-grpc-service";

  CheckGrpcService(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

  RunCheckResult RunCheck(
      CallContext& context,
      netwatch::monitor::v1::TargetIdRequest&& request) override;

  ListChecksResult ListChecks(
      CallContext& context,
      netwatch::monitor::v1::TargetIdRequest&& request) override;

  GetTargetStatusResult GetTargetStatus(
      CallContext& context,
      netwatch::monitor::v1::TargetIdRequest&& request) override;

 private:
  const CheckService& check_service_;
};

}  // namespace netwatch::monitor_service::checks
