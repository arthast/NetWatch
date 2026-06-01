#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include <netwatch/monitor_service_client.usrv.pb.hpp>
#include <userver/components/component_base.hpp>

#include <monitor_client/model/check_result.hpp>

namespace netwatch::monitor_client {

class CheckClient final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "check-client";

  CheckClient(const userver::components::ComponentConfig& config,
              const userver::components::ComponentContext& context);

  std::optional<CheckResult> RunCheck(std::int64_t target_id) const;

  std::optional<std::vector<CheckResult>> ListTargetChecks(
      std::int64_t target_id) const;

  std::optional<CheckResult> GetTargetStatus(std::int64_t target_id) const;

 private:
  netwatch::monitor::v1::CheckServiceClient& client_;
};

}  // namespace netwatch::monitor_client
