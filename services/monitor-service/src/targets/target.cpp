#include "target.hpp"

#include <stdexcept>

namespace monitor_service::target {
TargetType TargetTypeFromString(const std::string &value) {
    if (value == "http") {
        return TargetType::kHttp;
    }
    if (value == "tcp") {
        return TargetType::kTcp;
    }

    throw std::invalid_argument("Invalid target type: " + value);
}

std::string TargetTypeToString(TargetType type) {
    switch (type) {
        case TargetType::kHttp:
            return "http";
        case TargetType::kTcp:
            return "tcp";
    }

    throw std::invalid_argument("Invalid target type enum value");
}
} // namespace monitor_service::target
