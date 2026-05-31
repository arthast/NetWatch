#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include <userver/components/component_base.hpp>

#include <netwatch/target_service_client.usrv.pb.hpp>
#include <targets/model/target.hpp>

namespace userver::yaml_config {
class Schema;
}  // namespace userver::yaml_config

namespace netwatch::target_client {

class TargetClient final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "target-client";

  TargetClient(const userver::components::ComponentConfig& config,
               const userver::components::ComponentContext& context);

  static userver::yaml_config::Schema GetStaticConfigSchema();

  Target CreateTarget(const CreateTargetRequest& request) const;

  std::vector<Target> ListActiveTargets() const;

  std::optional<Target> GetTargetById(std::int64_t target_id) const;

  std::optional<Target> UpdateTarget(std::int64_t target_id,
                                     const UpdateTargetRequest& request) const;

  bool DeactivateTarget(std::int64_t target_id) const;

 private:
  netwatch::target::v1::TargetServiceClient* grpc_client_;
};

}  // namespace netwatch::target_client
