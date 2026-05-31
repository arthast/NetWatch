#include <checks/model/check_result.hpp>

#include <stdexcept>

namespace netwatch::monitor_service::checks {

std::string CheckStatusToString(CheckStatus status) {
  switch (status) {
    case CheckStatus::kUp:
      return "up";
    case CheckStatus::kDown:
      return "down";
  }

  throw std::invalid_argument("unknown check status");
}

CheckStatus CheckStatusFromString(const std::string& value) {
  if (value == "up") {
    return CheckStatus::kUp;
  }
  if (value == "down") {
    return CheckStatus::kDown;
  }

  throw std::invalid_argument("unknown check status: " + value);
}

std::string CheckProtocolToString(CheckProtocol protocol) {
  switch (protocol) {
    case CheckProtocol::kHttp:
      return "http";
    case CheckProtocol::kTcp:
      return "tcp";
  }

  throw std::invalid_argument("unknown check protocol");
}

CheckProtocol CheckProtocolFromString(const std::string& value) {
  if (value == "http") {
    return CheckProtocol::kHttp;
  }
  if (value == "tcp") {
    return CheckProtocol::kTcp;
  }

  throw std::invalid_argument("unknown check protocol: " + value);
}

}  // namespace netwatch::monitor_service::checks
