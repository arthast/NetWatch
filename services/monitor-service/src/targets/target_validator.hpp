#pragma once

#include <optional>
#include <string>

#include "target.hpp"

namespace monitor_service::target_validator {

std::optional<std::string> ValidateCreateTargetRequest(
    const target::CreateTargetRequest& request
);

}  // namespace monitor_service::target_validator
