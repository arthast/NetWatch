#pragma once

#include <optional>
#include <string>

#include <targets/model/target.hpp>

namespace netwatch::target_service::validator {
std::optional<std::string> ValidateCreateTargetRequest(
    const netwatch::target_service::CreateTargetRequest& request);

std::optional<std::string> ValidateTarget(
    const netwatch::target_service::Target& target);
}  // namespace netwatch::target_service::validator
