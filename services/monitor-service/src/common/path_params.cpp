#include <common/path_params.hpp>

#include <charconv>
#include <system_error>

namespace monitor_service::common {

std::optional<std::int64_t> ParsePositiveInt64(std::string_view value) {
  std::int64_t id = 0;
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto [ptr, error] = std::from_chars(begin, end, id);

  if (error != std::errc{} || ptr != end || id <= 0) {
    return std::nullopt;
  }

  return id;
}

}  // namespace monitor_service::common
