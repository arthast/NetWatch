#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/kafka/consumer_component.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <notifications/service/email_delivery_sender.hpp>
#include <notifications/service/notification_consumer.hpp>

int main(int argc, char* argv[]) {
  auto component_list =
      userver::components::MinimalServerComponentList()
          .Append<userver::server::handlers::Ping>()
          .Append<userver::components::TestsuiteSupport>()
          .AppendComponentList(userver::clients::http::ComponentList())
          .Append<userver::server::handlers::TestsControl>()
          .Append<userver::congestion_control::Component>()
          .Append<userver::clients::dns::Component>()
          .Append<userver::components::DefaultSecdistProvider>()
          .Append<userver::components::Secdist>()
          .Append<userver::kafka::ConsumerComponent>(
              "kafka-consumer-alert-events")
          .Append<userver::components::Postgres>("postgres-db-1")
          .Append<netwatch::notification_service::notifications::
                      EmailDeliverySender>()
          .Append<netwatch::notification_service::notifications::
                      NotificationConsumer>();

  return userver::utils::DaemonMain(argc, argv, component_list);
}
