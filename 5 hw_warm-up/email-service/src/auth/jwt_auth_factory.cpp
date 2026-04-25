#include "jwt_auth_factory.hpp"

namespace auth::jwt {

userver::server::handlers::auth::AuthCheckerBasePtr
JwtAuthCheckerFactory::operator()(
    const userver::components::ComponentContext& context,
    const userver::server::handlers::auth::HandlerAuthConfig&,
    const userver::server::handlers::auth::AuthCheckerSettings&) const {
  auto& auth = context.FindComponent<JwtAuthComponent>();
  return auth.Get();
}

}  // namespace auth::jwt
