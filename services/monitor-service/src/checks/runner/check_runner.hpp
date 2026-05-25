#pragma once

#include <userver/clients/dns/resolver.hpp>
#include <userver/clients/http/client.hpp>

#include <checks/model/check_result.hpp>
#include <targets/model/target.hpp>

namespace monitor_service::checks {
class CheckRunner {
public:
    CheckRunner(userver::clients::http::Client &http_client,
                userver::clients::dns::Resolver &dns_resolver);

    CheckResult RunCheck(const target::Target &target) const;

private:
    CheckResult RunHttpCheck(const target::Target &target) const;

    CheckResult RunTcpCheck(const target::Target &target) const;

    userver::clients::http::Client &http_client_;
    userver::clients::dns::Resolver &dns_resolver_;
};
} // namespace monitor_service::checks
