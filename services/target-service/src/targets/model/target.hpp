#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace netwatch::target_service {
enum class TargetType { kHttp, kTcp };

struct Target {
  std::int64_t id;
  std::string name;
  TargetType type;

  std::optional<std::string> url;
  std::optional<std::string> method;
  std::optional<int> expected_status_code;

  std::optional<std::string> host;
  std::optional<int> port;

  int interval_seconds;
  int timeout_ms;
  bool is_active;
};

struct CreateTargetRequest {
  std::string name;
  TargetType type;

  std::optional<std::string> url;
  std::optional<std::string> method;
  std::optional<int> expected_status_code;

  std::optional<std::string> host;
  std::optional<int> port;

  int interval_seconds;
  int timeout_ms;
};

struct UpdateTargetRequest {
  std::optional<std::string> name;
  std::optional<TargetType> type;

  std::optional<std::string> url;
  std::optional<std::string> method;
  std::optional<int> expected_status_code;

  std::optional<std::string> host;
  std::optional<int> port;

  std::optional<int> interval_seconds;
  std::optional<int> timeout_ms;
};

Target ApplyUpdate(Target target, const UpdateTargetRequest& request);

TargetType TargetTypeFromString(const std::string& value);

std::string TargetTypeToString(TargetType type);
}  // namespace netwatch::target_service
