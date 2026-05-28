#pragma once

#include <string_view>
#include <userver/components/component.hpp>

#include <netwatch/target_service_service.usrv.pb.hpp>
#include <targets/storage/target_repository.hpp>

namespace netwatch::target_service {

class TargetService final : public target::v1::TargetServiceBase::Component {
 public:
  static constexpr std::string_view kName = "target-service";

  TargetService(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context);

  CreateTargetResult CreateTarget(
      CallContext& context, target::v1::CreateTargetRequest&& request) override;

  UpdateTargetResult UpdateTarget(
      CallContext& context, target::v1::UpdateTargetRequest&& request) override;

  DeleteTargetResult DeleteTarget(
      CallContext& context, target::v1::TargetIdRequest&& request) override;

  GetTargetResult GetTarget(CallContext& context,
                            target::v1::TargetIdRequest&& request) override;

  ListTargetsResult ListTargets(
      CallContext& context, target::v1::ListTargetsRequest&& request) override;

  ListActiveTargetsResult ListActiveTargets(
      CallContext& context, target::v1::ListTargetsRequest&& request) override;

 private:
  monitor_service::target::TargetRepository repository_;
};

}  // namespace netwatch::target_service
