#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <monitor_client/client/check_client.hpp>
#include <monitor_client/model/check_result.hpp>
#include <target_client/client/target_client.hpp>

namespace netwatch::api_gateway::checks {

enum class UpstreamService { kMonitor, kTarget };

class UpstreamError final : public std::runtime_error {
 public:
  UpstreamError(UpstreamService service, bool is_deadline_exceeded,
                std::string message);

  UpstreamService GetService() const;

  bool IsDeadlineExceeded() const;

 private:
  UpstreamService service_;
  bool is_deadline_exceeded_;
};

enum class TargetStatusResultKind { kFound, kTargetNotFound, kNoChecks };

struct TargetStatusResult {
  TargetStatusResultKind kind;
  std::optional<netwatch::monitor_client::CheckResult> check;
};

class ChecksService final {
 public:
  ChecksService(const netwatch::monitor_client::CheckClient& check_client,
                const netwatch::target_client::TargetClient& target_client);

  std::optional<netwatch::monitor_client::CheckResult> RunCheck(
      std::int64_t user_id, std::int64_t target_id) const;

  std::optional<std::vector<netwatch::monitor_client::CheckResult>>
  ListTargetChecks(std::int64_t user_id, std::int64_t target_id) const;

  TargetStatusResult GetTargetStatus(std::int64_t user_id,
                                     std::int64_t target_id) const;

 private:
  const netwatch::monitor_client::CheckClient& check_client_;
  const netwatch::target_client::TargetClient& target_client_;
};

const char* ToString(UpstreamService service);

}  // namespace netwatch::api_gateway::checks
