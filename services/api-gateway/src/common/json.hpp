#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <userver/formats/json/value_builder.hpp>

namespace monitor_service::common {

template <typename T>
void SetOptionalField(userver::formats::json::ValueBuilder& builder,
                      std::string_view field, const std::optional<T>& value) {
  if (value) {
    builder[std::string{field}] = *value;
  }
}

}  // namespace monitor_service::common
