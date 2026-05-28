#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include <userver/components/component_base.hpp>

#include <netwatch/target_service_client.usrv.pb.hpp>
#include <targets/model/target.hpp>
#include <targets/storage/target_repository.hpp>

namespace userver::yaml_config {
class Schema;
}  // namespace userver::yaml_config

namespace monitor_service::target {

class TargetClient final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "target-client";

  TargetClient(const userver::components::ComponentConfig& config,
               const userver::components::ComponentContext& context);

  static userver::yaml_config::Schema GetStaticConfigSchema();

  Target CreateTarget(const CreateTargetRequest& request) const;

  std::vector<Target> ListActiveTargets() const;

  std::optional<Target> GetTargetById(std::int64_t target_id) const;

  std::optional<Target> UpdateTarget(const Target& target) const;

  bool DeactivateTarget(std::int64_t target_id) const;

 private:
  bool use_grpc_;
  netwatch::target::v1::TargetServiceClient* grpc_client_;
  std::optional<TargetRepository> repository_;
};

}  // namespace monitor_service::target
