#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <target_client/client/target_client.hpp>
#include <target_client/model/target.hpp>

namespace netwatch::api_gateway::targets {

class TargetsService final {
 public:
  explicit TargetsService(
      const netwatch::target_client::TargetClient& target_client);

  netwatch::target_client::Target CreateTarget(
      std::int64_t user_id,
      const netwatch::target_client::CreateTargetRequest& request) const;

  std::vector<netwatch::target_client::Target> ListActiveTargets(
      std::int64_t user_id) const;

  std::optional<netwatch::target_client::Target> GetTargetById(
      std::int64_t user_id, std::int64_t target_id) const;

  std::optional<netwatch::target_client::Target> UpdateTarget(
      std::int64_t user_id, std::int64_t target_id,
      const netwatch::target_client::UpdateTargetRequest& request) const;

  bool DeactivateTarget(std::int64_t user_id, std::int64_t target_id) const;

 private:
  const netwatch::target_client::TargetClient& target_client_;
};

}  // namespace netwatch::api_gateway::targets
