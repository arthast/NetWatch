#include <checks/service/check_service_component.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/component.hpp>

namespace netwatch::monitor_service::checks {

CheckServiceComponent::CheckServiceComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : ComponentBase(config, component_context),
      check_service_(
          component_context
              .FindComponent<netwatch::target_client::TargetClient>(),
          component_context
              .FindComponent<netwatch::alert_client::AlertClient>(),
          CheckRepository{
              component_context
                  .FindComponent<userver::components::Postgres>("postgres-db-1")
                  .GetCluster()},
          CheckRunner{
              component_context.FindComponent<userver::components::HttpClient>()
                  .GetHttpClient(),
              component_context
                  .FindComponent<userver::clients::dns::Component>()
                  .GetResolver()}) {}

const CheckService& CheckServiceComponent::GetService() const {
  return check_service_;
}

}  // namespace netwatch::monitor_service::checks
