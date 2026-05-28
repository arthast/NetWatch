#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace monitor_service::common {

std::optional<std::int64_t> ParsePositiveInt64(std::string_view value);

}  // namespace monitor_service::common
