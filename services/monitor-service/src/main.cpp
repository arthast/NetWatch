#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/storages/postgres/component.hpp>

#include <userver/utils/daemon_run.hpp>

#include <checks/handlers/manual_check_handler.hpp>
#include <checks/handlers/target_checks_handler.hpp>
#include <checks/handlers/target_status_handler.hpp>
#include <targets/handlers/target_by_id_handler.hpp>
#include <targets/handlers/targets_handler.hpp>

int main(int argc, char *argv[]) {
    auto component_list =
            userver::components::MinimalServerComponentList()
            .Append<userver::server::handlers::Ping>()
            .Append<userver::components::TestsuiteSupport>()
            .AppendComponentList(userver::clients::http::ComponentList())
            .Append<userver::clients::dns::Component>()
            .Append<userver::server::handlers::TestsControl>()
            .Append<userver::congestion_control::Component>()
            .Append<userver::components::Postgres>("postgres-db-1")
            .Append<monitor_service::checks::ManualCheckHandler>()
            .Append<monitor_service::checks::TargetChecksHandler>()
            .Append<monitor_service::checks::TargetStatusHandler>()
            .Append<monitor_service::target::TargetByIdHandler>()
            .Append<monitor_service::target::TargetsHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
