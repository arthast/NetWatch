#include <checks/model/check_result.hpp>

#include <stdexcept>

namespace netwatch::monitor_client {

std::string CheckStatusToString(CheckStatus status) {
  switch (status) {
    case CheckStatus::kUp:
      return "up";
    case CheckStatus::kDown:
      return "down";
  }

  throw std::invalid_argument("unknown check status");
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

}  // namespace netwatch::monitor_client
