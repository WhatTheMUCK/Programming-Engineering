#include "handlers.hpp"

#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <auth/jwt_auth_checker.hpp>
#include <auth/jwt_auth_factory.hpp>
#include "../common/mongo_component.hpp"

int main(int argc, char* argv[]) {
    userver::server::handlers::auth::RegisterAuthCheckerFactory(
        "jwt-auth", std::make_unique<auth::jwt::JwtAuthCheckerFactory>());
    
    auto component_list = userver::components::MinimalServerComponentList()
                              .Append<userver::server::handlers::Ping>()
                              .Append<userver::server::handlers::ServerMonitor>()
                              .Append<userver::components::TestsuiteSupport>()
                              .Append<userver::clients::dns::Component>()
                              .Append<email_service::MongoComponent>("mongo-db")
                              .Append<auth::jwt::JwtAuthComponent>();

    email_service::folder::AppendFolderHandlers(component_list);

    return userver::utils::DaemonMain(argc, argv, component_list);
}


