#include <targets/model/target.hpp>

#include <stdexcept>

namespace netwatch::target_service {
Target ApplyUpdate(Target target, const UpdateTargetRequest& request) {
  const auto previous_type = target.type;
  if (request.type) {
    target.type = *request.type;
  }

  if (target.type != previous_type) {
    if (target.type == TargetType::kHttp) {
      target.url = std::nullopt;
      target.method = "GET";
      target.expected_status_code = 200;
      target.host = std::nullopt;
      target.port = std::nullopt;
    } else {
      target.url = std::nullopt;
      target.method = std::nullopt;
      target.expected_status_code = std::nullopt;
      target.host = std::nullopt;
      target.port = std::nullopt;
    }
  }

  if (request.name) {
    target.name = *request.name;
  }
  if (request.url) {
    target.url = *request.url;
  }
  if (request.method) {
    target.method = *request.method;
  }
  if (request.expected_status_code) {
    target.expected_status_code = *request.expected_status_code;
  }
  if (request.host) {
    target.host = *request.host;
  }
  if (request.port) {
    target.port = *request.port;
  }
  if (request.interval_seconds) {
    target.interval_seconds = *request.interval_seconds;
  }
  if (request.timeout_ms) {
    target.timeout_ms = *request.timeout_ms;
  }

  return target;
}

TargetType TargetTypeFromString(const std::string& value) {
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
}  // namespace netwatch::target_service
