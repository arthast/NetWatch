#include <targets/service/target_service.hpp>

#include <optional>
#include <utility>

#include <targets/validation/target_validator.hpp>

namespace netwatch::target_service {
namespace {

void ApplyCreateDefaults(CreateTargetRequest& request) {
  if (request.type != TargetType::kHttp) {
    return;
  }

  if (!request.method) {
    request.method = "GET";
  }
  if (!request.expected_status_code) {
    request.expected_status_code = 200;
  }
}

bool HasPatchFields(const UpdateTargetRequest& request) {
  return request.name || request.type || request.url || request.method ||
         request.expected_status_code || request.host || request.port ||
         request.interval_seconds || request.timeout_ms;
}

void ValidateCreateRequest(const CreateTargetRequest& request) {
  if (const auto error = validator::ValidateCreateTargetRequest(request)) {
    throw std::invalid_argument(*error);
  }
}

void ValidateTargetForUpdate(const Target& target) {
  if (const auto error = validator::ValidateTarget(target)) {
    throw std::invalid_argument(*error);
  }
}

}  // namespace

TargetNotFound::TargetNotFound() : std::runtime_error("target not found") {}

TargetService::TargetService(TargetRepository repository)
    : repository_(std::move(repository)) {}

Target TargetService::CreateTarget(CreateTargetRequest request) const {
  ApplyCreateDefaults(request);
  ValidateCreateRequest(request);

  if (const auto existing = repository_.FindActiveEquivalentTarget(request)) {
    return *existing;
  }

  return repository_.CreateTarget(request);
}

Target TargetService::UpdateTarget(std::int64_t target_id,
                                   const UpdateTargetRequest& request) const {
  if (target_id <= 0) {
    throw std::invalid_argument("target id must be a positive integer");
  }
  if (!HasPatchFields(request)) {
    throw std::invalid_argument("patch body must contain at least one field");
  }

  const auto current_target = repository_.GetTargetById(target_id);
  if (!current_target) {
    throw TargetNotFound{};
  }

  const auto target = ApplyUpdate(*current_target, request);
  ValidateTargetForUpdate(target);

  const auto updated = repository_.UpdateTarget(target);
  if (!updated) {
    throw TargetNotFound{};
  }

  return *updated;
}

void TargetService::DeleteTarget(std::int64_t target_id) const {
  if (!repository_.DeactivateTarget(target_id)) {
    throw TargetNotFound{};
  }
}

Target TargetService::GetTarget(std::int64_t target_id) const {
  const auto target = repository_.GetTargetById(target_id);
  if (!target) {
    throw TargetNotFound{};
  }

  return *target;
}

std::vector<Target> TargetService::ListTargets() const {
  return repository_.ListTargets();
}

std::vector<Target> TargetService::ListActiveTargets() const {
  return repository_.ListActiveTargets();
}

}  // namespace netwatch::target_service
