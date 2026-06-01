#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/client/component_list.hpp>
#include <userver/ugrpc/client/simple_client_component.hpp>
#include <userver/ugrpc/server/component_list.hpp>

#include <netwatch/alert_service_client.usrv.pb.hpp>
#include <netwatch/target_service_client.usrv.pb.hpp>
#include <userver/storages/postgres/component.hpp>

#include <userver/utils/daemon_run.hpp>

#include <alert_client/client/alert_client.hpp>
#include <checks/grpc/check_grpc_service.hpp>
#include <checks/scheduler/target_check_scheduler.hpp>
#include <checks/service/check_service.hpp>
#include <target_client/client/target_client.hpp>

int main(int argc, char* argv[]) {
  auto component_list =
      userver::components::MinimalServerComponentList()
          .Append<userver::server::handlers::Ping>()
          .Append<userver::components::TestsuiteSupport>()
          .AppendComponentList(userver::clients::http::ComponentList())
          .Append<userver::clients::dns::Component>()
          .Append<userver::server::handlers::TestsControl>()
          .Append<userver::congestion_control::Component>()
          .Append<userver::components::Postgres>("postgres-db-1")
          .AppendComponentList(userver::ugrpc::client::MinimalComponentList())
          .Append<userver::ugrpc::client::ClientFactoryComponent>()
          .Append<userver::ugrpc::client::SimpleClientComponent<
              netwatch::target::v1::TargetServiceClient>>(
              "target-service-client")
          .Append<userver::ugrpc::client::SimpleClientComponent<
              netwatch::alert::v1::AlertServiceClient>>("alert-service-client")
          .AppendComponentList(userver::ugrpc::server::MinimalComponentList())
          .Append<netwatch::target_client::TargetClient>()
          .Append<netwatch::alert_client::AlertClient>()
          .Append<netwatch::monitor_service::checks::CheckServiceComponent>()
          .Append<netwatch::monitor_service::checks::TargetCheckScheduler>()
          .Append<netwatch::monitor_service::checks::CheckGrpcService>();

  return userver::utils::DaemonMain(argc, argv, component_list);
}
