#pragma once

#include <memory>

#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "../common/database.hpp"
#include "../common/models.hpp"
#include "../common/cache_component.hpp"
#include "../common/rate_limiter.hpp"

namespace email_service::user {

// POST /api/v1/users - Create new user
class CreateUserHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-create-user";

    CreateUserHandler(const userver::components::ComponentConfig& config,
                     const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    std::shared_ptr<Database> db_;
    CacheComponent* cache_ = nullptr;
    RateLimiterComponent* rate_limiter_ = nullptr;
};

// GET /api/v1/users/by-login?login=... - Find user by login
class FindUserByLoginHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-find-user-by-login";

    FindUserByLoginHandler(const userver::components::ComponentConfig& config,
                           const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    std::shared_ptr<Database> db_;
    CacheComponent* cache_ = nullptr;
};

// GET /api/v1/users/search?mask=... - Search users by name mask
class SearchUsersByNameHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-search-users-by-name";

    SearchUsersByNameHandler(const userver::components::ComponentConfig& config,
                            const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    std::shared_ptr<Database> db_;
    CacheComponent* cache_ = nullptr;
};

// POST /api/v1/auth/login - Login user
class LoginHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-login";

    LoginHandler(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override;

private:
    std::shared_ptr<Database> db_;
    RateLimiterComponent* rate_limiter_ = nullptr;
    std::string GenerateToken(const std::string& user_id) const;
};

void AppendUserHandlers(userver::components::ComponentList& component_list);

}  // namespace email_service::user