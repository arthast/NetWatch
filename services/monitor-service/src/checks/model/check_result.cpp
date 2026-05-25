#include <checks/model/check_result.hpp>

#include <stdexcept>

namespace monitor_service::checks {

std::string CheckStatusToString(CheckStatus status) {
    switch (status) {
        case CheckStatus::kUp:
            return "up";
        case CheckStatus::kDown:
            return "down";
    }

    throw std::invalid_argument("unknown check status");
}

CheckStatus CheckStatusFromString(const std::string &value) {
    if (value == "up") {
        return CheckStatus::kUp;
    }
    if (value == "down") {
        return CheckStatus::kDown;
    }

    throw std::invalid_argument("unknown check status: " + value);
}

} // namespace monitor_service::checks
