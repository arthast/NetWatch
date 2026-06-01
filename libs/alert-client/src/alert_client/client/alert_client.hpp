#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <netwatch/alert_service_client.usrv.pb.hpp>
#include <userver/components/component_base.hpp>

#include <alert_client/model/alert.hpp>

namespace netwatch::alert_client {

enum class TargetType { kHttp, kTcp };

struct TargetSnapshot {
  std::int64_t id{0};
  std::string name;
  TargetType type{TargetType::kHttp};

  std::optional<std::string> url;
  std::optional<std::string> method;
  std::optional<int> expected_status_code;

  std::optional<std::string> host;
  std::optional<int> port;

  int interval_seconds{0};
  int timeout_ms{0};
  bool is_active{false};
};

enum class CheckStatus { kUp, kDown };

struct CheckResultSnapshot {
  std::int64_t id{0};
  std::int64_t target_id{0};
  CheckStatus status{CheckStatus::kDown};
  TargetType protocol{TargetType::kHttp};

  std::optional<int> http_status;
  std::optional<int> latency_ms;
  std::optional<std::string> error_message;
  std::string checked_at;
};

class AlertClient final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "alert-client";

  AlertClient(const userver::components::ComponentConfig& config,
              const userver::components::ComponentContext& context);

  std::vector<Alert> ListAlerts() const;

  std::vector<Alert> ListActiveAlerts() const;

  void ProcessCheckResult(
      const TargetSnapshot& target,
      const std::optional<CheckResultSnapshot>& previous_check,
      const CheckResultSnapshot& current_check) const;

 private:
  netwatch::alert::v1::AlertServiceClient& client_;
};

}  // namespace netwatch::alert_client
