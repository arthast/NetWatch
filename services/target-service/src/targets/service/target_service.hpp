#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <targets/model/target.hpp>
#include <targets/storage/target_repository.hpp>

namespace netwatch::target_service {

class TargetNotFound final : public std::runtime_error {
 public:
  TargetNotFound();
};

class TargetService {
 public:
  explicit TargetService(TargetRepository repository);

  Target CreateTarget(CreateTargetRequest request) const;

  Target UpdateTarget(std::int64_t target_id,
                      const UpdateTargetRequest& request) const;

  void DeleteTarget(std::int64_t target_id) const;

  void DeleteTargetForUser(std::int64_t target_id, std::int64_t user_id) const;

  Target GetTarget(std::int64_t target_id) const;

  Target GetTargetForUser(std::int64_t target_id, std::int64_t user_id) const;

  std::vector<Target> ListTargets() const;

  std::vector<Target> ListActiveTargets() const;

  std::vector<Target> ListActiveTargetsForUser(std::int64_t user_id) const;

 private:
  TargetRepository repository_;
};

}  // namespace netwatch::target_service
