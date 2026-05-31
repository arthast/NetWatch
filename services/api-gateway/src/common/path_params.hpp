#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace netwatch::api_gateway::common {

std::optional<std::int64_t> ParsePositiveInt64(std::string_view value);

}  // namespace netwatch::api_gateway::common
