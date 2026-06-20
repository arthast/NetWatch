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
    std::int64_t user_id,
    const netwatch::target_client::CreateTargetRequest& request) const {
  auto scoped_request = request;
  scoped_request.user_id = user_id;
  return target_client_.CreateTarget(scoped_request);
}

std::vector<netwatch::target_client::Target> TargetsService::ListActiveTargets(
    std::int64_t user_id) const {
  return target_client_.ListActiveTargetsForUser(user_id);
}

std::optional<netwatch::target_client::Target> TargetsService::GetTargetById(
    std::int64_t user_id, std::int64_t target_id) const {
  return target_client_.GetTargetByIdForUser(target_id, user_id);
}

std::optional<netwatch::target_client::Target> TargetsService::UpdateTarget(
    std::int64_t user_id, std::int64_t target_id,
    const netwatch::target_client::UpdateTargetRequest& request) const {
  if (!HasPatchFields(request)) {
    throw std::invalid_argument{"patch body must contain at least one field"};
  }

  return target_client_.UpdateTargetForUser(target_id, user_id, request);
}

bool TargetsService::DeactivateTarget(std::int64_t user_id,
                                      std::int64_t target_id) const {
  return target_client_.DeactivateTargetForUser(target_id, user_id);
}

}  // namespace netwatch::api_gateway::targets
