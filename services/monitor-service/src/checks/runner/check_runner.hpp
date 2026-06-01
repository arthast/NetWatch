#pragma once

#include <userver/clients/dns/resolver.hpp>
#include <userver/clients/http/client.hpp>

#include <checks/model/check_result.hpp>
#include <target_client/model/target.hpp>

namespace netwatch::monitor_service::checks {
class CheckRunner {
 public:
  CheckRunner(userver::clients::http::Client& http_client,
              userver::clients::dns::Resolver& dns_resolver);

  CheckResult RunCheck(const netwatch::target_client::Target& target) const;

 private:
  CheckResult RunHttpCheck(const netwatch::target_client::Target& target) const;

  CheckResult RunTcpCheck(const netwatch::target_client::Target& target) const;

  userver::clients::http::Client& http_client_;
  userver::clients::dns::Resolver& dns_resolver_;
};
}  // namespace netwatch::monitor_service::checks
