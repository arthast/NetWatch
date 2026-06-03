#include <checks/service/checks_service.hpp>

#include <userver/ugrpc/client/exceptions.hpp>
#include <utility>

namespace netwatch::api_gateway::checks {
namespace {

bool IsDeadlineExceeded(const userver::ugrpc::client::BaseError& error) {
  return dynamic_cast<const userver::ugrpc::client::DeadlineExceededError*>(
             &error) != nullptr;
}

}  // namespace

UpstreamError::UpstreamError(UpstreamService service, bool is_deadline_exceeded,
                             std::string message)
    : std::runtime_error(std::move(message)),
      service_(service),
      is_deadline_exceeded_(is_deadline_exceeded) {}

UpstreamService UpstreamError::GetService() const { return service_; }

bool UpstreamError::IsDeadlineExceeded() const { return is_deadline_exceeded_; }

ChecksService::ChecksService(
    const netwatch::monitor_client::CheckClient& check_client,
    const netwatch::target_client::TargetClient& target_client)
    : check_client_(check_client), target_client_(target_client) {}

std::optional<netwatch::monitor_client::CheckResult> ChecksService::RunCheck(
    std::int64_t target_id) const {
  return check_client_.RunCheck(target_id);
}

std::optional<std::vector<netwatch::monitor_client::CheckResult>>
ChecksService::ListTargetChecks(std::int64_t target_id) const {
  return check_client_.ListTargetChecks(target_id);
}

TargetStatusResult ChecksService::GetTargetStatus(
    std::int64_t target_id) const {
  const auto status = check_client_.GetTargetStatus(target_id);
  if (status) {
    return TargetStatusResult{
        .kind = TargetStatusResultKind::kFound,
        .check = status,
    };
  }

  try {
    if (!target_client_.GetTargetById(target_id)) {
      return TargetStatusResult{
          .kind = TargetStatusResultKind::kTargetNotFound,
          .check = std::nullopt,
      };
    }
  } catch (const userver::ugrpc::client::BaseError& ex) {
    throw UpstreamError{UpstreamService::kTarget, IsDeadlineExceeded(ex),
                        ex.what()};
  }

  return TargetStatusResult{
      .kind = TargetStatusResultKind::kNoChecks,
      .check = std::nullopt,
  };
}

const char* ToString(UpstreamService service) {
  switch (service) {
    case UpstreamService::kMonitor:
      return "monitor-service";
    case UpstreamService::kTarget:
      return "target-service";
  }

  return "unknown-service";
}

}  // namespace netwatch::api_gateway::checks
