#include "handlers.hpp"

#include <userver/components/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/utils/datetime.hpp>
#include <jwt-cpp/jwt.h>

#include "../common/mongo_component.hpp"

namespace email_service::user {

CreateUserHandler::CreateUserHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    auto& mongo_component = context.FindComponent<MongoComponent>("mongo-db");
    db_ = mongo_component.GetDatabase();
    
    // Get cache and rate limiter components if available
    try {
        cache_ = &context.FindComponent<CacheComponent>("cache");
    } catch (...) {
        cache_ = nullptr;
    }
    try {
        rate_limiter_ = &context.FindComponent<RateLimiterComponent>("rate-limiter");
    } catch (...) {
        rate_limiter_ = nullptr;
    }
}

std::string CreateUserHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    
    // Rate limiting check
    if (rate_limiter_) {
        auto client_ip = request.GetRemoteAddress().PrimaryAddressString();
        if (!rate_limiter_->CheckUserRegistration(client_ip)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kTooManyRequests);
            auto& response = request.GetHttpResponse();
            SetRateLimitHeaders(response, rate_limiter_->GetRateLimitInfo(
                "user_reg:" + client_ip, {10, std::chrono::seconds(60), 5}));
            response.SetHeader(std::string{"Retry-After"}, std::string{"60"});
            return userver::formats::json::ToString(
                userver::formats::json::MakeObject("error", "Too many registration attempts. Please try again later.")
            );
        }
    }
    
    auto request_body = userver::formats::json::FromString(request.RequestBody());
    
    if (!request_body.HasMember("login") || !request_body.HasMember("email") ||
        !request_body.HasMember("first_name") || !request_body.HasMember("last_name") ||
        !request_body.HasMember("password")) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Missing required fields")
        );
    }
    
    auto create_request = CreateUserRequest::FromJson(request_body);

    if (create_request.login.empty() || create_request.email.empty() ||
        create_request.first_name.empty() || create_request.last_name.empty() ||
        create_request.password.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Missing required fields")
        );
    }
    
    if (create_request.email.find('@') == std::string::npos) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Invalid email format")
        );
    }

    auto user = db_->CreateUser(create_request);
    if (!user) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "User already exists")
        );
    }

    // Invalidate cache for this user
    if (cache_) {
        cache_->GetUserCache()->Invalidate("user:login:" + create_request.login);
    }

    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    return userver::formats::json::ToString(user->ToJson());
}

FindUserByLoginHandler::FindUserByLoginHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    auto& mongo_component = context.FindComponent<MongoComponent>("mongo-db");
    db_ = mongo_component.GetDatabase();
    
    // Get cache component if available
    try {
        cache_ = &context.FindComponent<CacheComponent>("cache");
    } catch (...) {
        cache_ = nullptr;
    }
}

std::string FindUserByLoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    
    auto login = request.GetArg("login");
    if (login.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Missing 'login' parameter")
        );
    }

    // Check cache first
    std::string cache_key = "user:login:" + login;
    if (cache_) {
        auto cached_user = cache_->GetUserCache()->Get(cache_key);
        if (cached_user) {
            cache_->GetUserCacheStats().RecordHit();
            return *cached_user;
        }
        cache_->GetUserCacheStats().RecordMiss();
    }

    auto user = db_->FindUserByLogin(login);
    if (!user) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "User not found")
        );
    }

    // Cache the result
    if (cache_) {
        auto user_json = userver::formats::json::ToString(user->ToJson());
        cache_->GetUserCache()->Put(cache_key, user_json);
    }

    return userver::formats::json::ToString(user->ToJson());
}

SearchUsersByNameHandler::SearchUsersByNameHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    auto& mongo_component = context.FindComponent<MongoComponent>("mongo-db");
    db_ = mongo_component.GetDatabase();
    
    // Get cache component if available
    try {
        cache_ = &context.FindComponent<CacheComponent>("cache");
    } catch (...) {
        cache_ = nullptr;
    }
}

std::string SearchUsersByNameHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    
    auto mask = request.GetArg("mask");
    if (mask.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Missing 'mask' parameter")
        );
    }

    // Check cache first
    std::string cache_key = "user:search:" + mask;
    if (cache_) {
        auto cached_users = cache_->GetUserCache()->Get(cache_key);
        if (cached_users) {
            cache_->GetUserCacheStats().RecordHit();
            return *cached_users;
        }
        cache_->GetUserCacheStats().RecordMiss();
    }

    auto users = db_->SearchUsersByNameMask(mask);
    
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kArray);
    for (const auto& user : users) {
        builder.PushBack(user.ToJson());
    }

    auto result = userver::formats::json::ToString(builder.ExtractValue());

    // Cache the result
    if (cache_) {
        cache_->GetUserCache()->Put(cache_key, result, std::chrono::seconds(120)); // 2 minutes TTL
    }

    return result;
}

LoginHandler::LoginHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    auto& mongo_component = context.FindComponent<MongoComponent>("mongo-db");
    db_ = mongo_component.GetDatabase();
    
    // Get rate limiter component if available
    try {
        rate_limiter_ = &context.FindComponent<RateLimiterComponent>("rate-limiter");
    } catch (...) {
        rate_limiter_ = nullptr;
    }
}

std::string LoginHandler::GenerateToken(const std::string& user_id) const {
    // Generate JWT token with 24 hour expiration using kazuho-picojson traits
    // user_id is MongoDB ObjectId as string
    auto token = ::jwt::create<::jwt::traits::kazuho_picojson>()
        .set_issuer("email-service")
        .set_type("JWT")
        .set_payload_claim("user_id", ::jwt::claim(user_id))
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24))
        .sign(::jwt::algorithm::hs256{"your-secret-key-change-in-production"});
    
    return token;
}

std::string LoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    
    auto request_body = userver::formats::json::FromString(request.RequestBody());
    
    // Check for missing required fields
    if (!request_body.HasMember("login")) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Login is required")
        );
    }
    if (!request_body.HasMember("password")) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Password is required")
        );
    }
    
    auto login_request = LoginRequest::FromJson(request_body);

    // Validate required fields
    if (login_request.login.empty() || login_request.password.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Missing required fields")
        );
    }

    // Rate limiting check
    if (rate_limiter_) {
        auto client_ip = request.GetRemoteAddress().PrimaryAddressString();
        if (!rate_limiter_->CheckLogin(client_ip)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kTooManyRequests);
            auto& response = request.GetHttpResponse();
            SetRateLimitHeaders(response, rate_limiter_->GetRateLimitInfo(
                "login:" + client_ip, {20, std::chrono::seconds(60), 5}));
            response.SetHeader(std::string{"Retry-After"}, std::string{"60"});
            return userver::formats::json::ToString(
                userver::formats::json::MakeObject("error", "Too many login attempts. Please try again later.")
            );
        }
    }

    auto user = db_->AuthenticateUser(login_request.login, login_request.password);
    if (!user) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Invalid credentials")
        );
    }

    AuthToken token;
    token.token = GenerateToken(user->id);  // ObjectId string
    token.user_id = user->id;  // ObjectId string
    token.expires_at = std::chrono::system_clock::now() + std::chrono::hours(24);

    return userver::formats::json::ToString(token.ToJson());
}

void AppendUserHandlers(userver::components::ComponentList& component_list) {
    component_list.Append<CreateUserHandler>();
    component_list.Append<FindUserByLoginHandler>();
    component_list.Append<SearchUsersByNameHandler>();
    component_list.Append<LoginHandler>();
}

}  // namespace email_service::user