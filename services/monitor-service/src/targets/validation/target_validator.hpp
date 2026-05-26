#pragma once

#include <optional>
#include <string>

#include <targets/model/target.hpp>

namespace monitor_service::target_validator {
std::optional<std::string> ValidateCreateTargetRequest(
    const target::CreateTargetRequest &request
);

std::optional<std::string> ValidateTarget(const target::Target &target);
} // namespace monitor_service::target_validator
