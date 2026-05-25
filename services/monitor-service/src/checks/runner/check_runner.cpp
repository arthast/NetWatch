#include <checks/runner/check_runner.hpp>

#include <chrono>
#include <exception>
#include <optional>
#include <string>
#include <userver/clients/dns/exception.hpp>
#include <userver/clients/http/error.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/socket.hpp>

namespace monitor_service::checks {
namespace {
    int ElapsedMs(std::chrono::steady_clock::time_point started_at) {
        const auto elapsed = std::chrono::steady_clock::now() - started_at;
        return static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
    }

    CheckResult MakeBaseResult(const target::Target &target) {
        return CheckResult{
            .target_id = target.id,
            .status = CheckStatus::kDown,
            .protocol = target.type,
            .http_status = std::nullopt,
            .latency_ms = std::nullopt,
            .error_message = std::nullopt,
            .checked_at = {},
        };
    }
} // namespace

CheckRunner::CheckRunner(userver::clients::http::Client &http_client,
                         userver::clients::dns::Resolver &dns_resolver)
    : http_client_(http_client), dns_resolver_(dns_resolver) {
}

CheckResult CheckRunner::RunCheck(const target::Target &target) const {
    switch (target.type) {
        case target::TargetType::kHttp:
            return RunHttpCheck(target);
        case target::TargetType::kTcp:
            return RunTcpCheck(target);
    }

    auto result = MakeBaseResult(target);
    result.error_message = "unknown target type";
    return result;
}

CheckResult CheckRunner::RunHttpCheck(const target::Target &target) const {
    auto result = MakeBaseResult(target);
    const auto started_at = std::chrono::steady_clock::now();

    try {
        const auto method =
                userver::clients::http::HttpMethodFromString(*target.method);
        const auto response = http_client_.CreateRequest()
                .method(method)
                .url(*target.url)
                .timeout(std::chrono::milliseconds{target.timeout_ms})
                .retry(0, false)
                .perform();

        const auto status_code = static_cast<int>(response->status_code());
        result.http_status = status_code;
        result.latency_ms = ElapsedMs(started_at);
        result.status = status_code == *target.expected_status_code
                            ? CheckStatus::kUp
                            : CheckStatus::kDown;

        if (result.status == CheckStatus::kDown) {
            result.error_message =
                    "unexpected HTTP status: " + std::to_string(status_code);
        }
    } catch (const userver::clients::http::TimeoutException &ex) {
        result.latency_ms = ElapsedMs(started_at);
        result.error_message = ex.what();
    } catch (const userver::clients::http::BaseException &ex) {
        result.latency_ms = ElapsedMs(started_at);
        result.error_message = ex.what();
    } catch (const std::exception &ex) {
        result.latency_ms = ElapsedMs(started_at);
        result.error_message = ex.what();
    }

    return result;
}

CheckResult CheckRunner::RunTcpCheck(const target::Target &target) const {
    auto result = MakeBaseResult(target);
    const auto started_at = std::chrono::steady_clock::now();
    const auto deadline =
            userver::engine::Deadline::FromDuration(std::chrono::milliseconds{target.timeout_ms});

    try {
        auto addresses = dns_resolver_.Resolve(*target.host, deadline);
        if (addresses.empty()) {
            result.error_message = "host has no resolved addresses";
            result.latency_ms = ElapsedMs(started_at);
            return result;
        }

        std::string last_error;
        for (auto address: addresses) {
            try {
                address.SetPort(static_cast<std::uint16_t>(*target.port));
                userver::engine::io::Socket socket(
                    address.Domain(), userver::engine::io::SocketType::kTcp);
                socket.Connect(address, deadline);
                socket.Close();

                result.status = CheckStatus::kUp;
                result.latency_ms = ElapsedMs(started_at);
                result.error_message = std::nullopt;
                return result;
            } catch (const userver::engine::io::IoException &ex) {
                last_error = ex.what();
            }
        }

        result.latency_ms = ElapsedMs(started_at);
        result.error_message =
                last_error.empty() ? "tcp connection failed" : last_error;
    } catch (const userver::clients::dns::ResolverException &ex) {
        result.latency_ms = ElapsedMs(started_at);
        result.error_message = ex.what();
    } catch (const std::exception &ex) {
        result.latency_ms = ElapsedMs(started_at);
        result.error_message = ex.what();
    }

    return result;
}
} // namespace monitor_service::checks
