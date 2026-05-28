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

#include <netwatch/monitor_service_client.usrv.pb.hpp>
#include <netwatch/target_service_client.usrv.pb.hpp>

#include <alerts/client/alert_client.hpp>
#include <alerts/handlers/active_alerts_handler.hpp>
#include <alerts/handlers/alerts_handler.hpp>
#include <checks/client/check_client.hpp>
#include <checks/handlers/manual_check_handler.hpp>
#include <checks/handlers/target_checks_handler.hpp>
#include <checks/handlers/target_status_handler.hpp>
#include <targets/client/target_client.hpp>
#include <targets/handlers/target_by_id_handler.hpp>
#include <targets/handlers/targets_handler.hpp>
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
              netwatch::monitor::v1::AlertServiceClient>>(
              "alert-service-client")
          .Append<monitor_service::target::TargetClient>()
          .Append<monitor_service::checks::CheckClient>()
          .Append<monitor_service::alerts::AlertClient>()
          .Append<monitor_service::web::SwaggerUiHandler>()
          .Append<monitor_service::web::OpenApiHandler>()
          .Append<monitor_service::alerts::AlertsHandler>()
          .Append<monitor_service::alerts::ActiveAlertsHandler>()
          .Append<monitor_service::checks::ManualCheckHandler>()
          .Append<monitor_service::checks::TargetChecksHandler>()
          .Append<monitor_service::checks::TargetStatusHandler>()
          .Append<monitor_service::target::TargetByIdHandler>()
          .Append<monitor_service::target::TargetsHandler>();

  return userver::utils::DaemonMain(argc, argv, component_list);
}
