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
#include <userver/utils/daemon_run.hpp>

#include <netwatch/alert_service_client.usrv.pb.hpp>
#include <netwatch/monitor_service_client.usrv.pb.hpp>
#include <netwatch/target_service_client.usrv.pb.hpp>

#include <alert_client/client/alert_client.hpp>
#include <alerts/handlers/active_alerts_handler.hpp>
#include <alerts/handlers/alerts_handler.hpp>
#include <alerts/service/alerts_service_component.hpp>
#include <checks/handlers/manual_check_handler.hpp>
#include <checks/handlers/target_checks_handler.hpp>
#include <checks/handlers/target_status_handler.hpp>
#include <checks/service/checks_service_component.hpp>
#include <monitor_client/client/check_client.hpp>
#include <target_client/client/target_client.hpp>
#include <targets/handlers/target_by_id_handler.hpp>
#include <targets/handlers/targets_handler.hpp>
#include <targets/service/targets_service_component.hpp>
#include <web/handlers/openapi_handler.hpp>
#include <web/handlers/swagger_ui_handler.hpp>

int main(int argc, char* argv[]) {
  auto component_list =
      userver::components::MinimalServerComponentList()
          .Append<userver::server::handlers::Ping>()
          .Append<userver::components::TestsuiteSupport>()
          .Append<userver::server::handlers::TestsControl>()
          .Append<userver::congestion_control::Component>()
          .AppendComponentList(userver::ugrpc::client::MinimalComponentList())
          .Append<userver::ugrpc::client::ClientFactoryComponent>()
          .Append<userver::ugrpc::client::SimpleClientComponent<
              netwatch::target::v1::TargetServiceClient>>(
              "target-service-client")
          .Append<userver::ugrpc::client::SimpleClientComponent<
              netwatch::monitor::v1::CheckServiceClient>>(
              "check-service-client")
          .Append<userver::ugrpc::client::SimpleClientComponent<
              netwatch::alert::v1::AlertServiceClient>>("alert-service-client")
          .Append<netwatch::target_client::TargetClient>()
          .Append<netwatch::monitor_client::CheckClient>()
          .Append<netwatch::alert_client::AlertClient>()
          .Append<netwatch::api_gateway::targets::TargetsServiceComponent>()
          .Append<netwatch::api_gateway::checks::ChecksServiceComponent>()
          .Append<netwatch::api_gateway::alerts::AlertsServiceComponent>()
          .Append<netwatch::api_gateway::web::SwaggerUiHandler>()
          .Append<netwatch::api_gateway::web::OpenApiHandler>()
          .Append<netwatch::api_gateway::alerts::AlertsHandler>()
          .Append<netwatch::api_gateway::alerts::ActiveAlertsHandler>()
          .Append<netwatch::api_gateway::checks::ManualCheckHandler>()
          .Append<netwatch::api_gateway::checks::TargetChecksHandler>()
          .Append<netwatch::api_gateway::checks::TargetStatusHandler>()
          .Append<netwatch::api_gateway::targets::TargetByIdHandler>()
          .Append<netwatch::api_gateway::targets::TargetsHandler>();

  return userver::utils::DaemonMain(argc, argv, component_list);
}
