#include <targets/service/targets_service.hpp>

#include <stdexcept>

namespace netwatch::api_gateway::targets {
namespace {

bool HasPatchFields(
    const netwatch::target_client::UpdateTargetRequest& request) {
  return request.name || request.type || request.url || request.method ||
         request.expected_status_code || request.host || request.port ||
         request.interval_seconds || request.timeout_ms;
}

}  // namespace

TargetsService::TargetsService(
    const netwatch::target_client::TargetClient& target_client)
    : target_client_(target_client) {}

netwatch::target_client::Target TargetsService::CreateTarget(
    const netwatch::target_client::CreateTargetRequest& request) const {
  return target_client_.CreateTarget(request);
}

std::vector<netwatch::target_client::Target> TargetsService::ListActiveTargets()
    const {
  return target_client_.ListActiveTargets();
}

std::optional<netwatch::target_client::Target> TargetsService::GetTargetById(
    std::int64_t target_id) const {
  return target_client_.GetTargetById(target_id);
}

std::optional<netwatch::target_client::Target> TargetsService::UpdateTarget(
    std::int64_t target_id,
    const netwatch::target_client::UpdateTargetRequest& request) const {
  if (!HasPatchFields(request)) {
    throw std::invalid_argument{"patch body must contain at least one field"};
  }

  return target_client_.UpdateTarget(target_id, request);
}

bool TargetsService::DeactivateTarget(std::int64_t target_id) const {
  return target_client_.DeactivateTarget(target_id);
}

}  // namespace netwatch::api_gateway::targets
