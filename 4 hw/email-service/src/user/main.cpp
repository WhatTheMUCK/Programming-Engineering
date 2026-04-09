#include "handlers.hpp"

#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "../common/database.hpp"

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList()
                              .Append<userver::server::handlers::Ping>()
                              .Append<userver::components::TestsuiteSupport>()
                              .Append<userver::clients::dns::Component>()
                              .Append<userver::components::Postgres>("postgres-db");

    email_service::user::AppendUserHandlers(component_list);

    return userver::utils::DaemonMain(argc, argv, component_list);
}