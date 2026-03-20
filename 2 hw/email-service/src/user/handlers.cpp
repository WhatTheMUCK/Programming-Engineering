#include "handlers.hpp"

#include <userver/components/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/utils/datetime.hpp>
#include <jwt-cpp/jwt.h>

namespace email_service::user {

CreateUserHandler::CreateUserHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    db_ = std::make_shared<Database>("/app/data/email.db");
}

std::string CreateUserHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    
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

    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    return userver::formats::json::ToString(user->ToJson());
}

FindUserByLoginHandler::FindUserByLoginHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    db_ = std::make_shared<Database>("/app/data/email.db");
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

    auto user = db_->FindUserByLogin(login);
    if (!user) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "User not found")
        );
    }

    return userver::formats::json::ToString(user->ToJson());
}

SearchUsersByNameHandler::SearchUsersByNameHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    db_ = std::make_shared<Database>("/app/data/email.db");
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

    auto users = db_->SearchUsersByNameMask(mask);
    
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kArray);
    for (const auto& user : users) {
        builder.PushBack(user.ToJson());
    }

    return userver::formats::json::ToString(builder.ExtractValue());
}

LoginHandler::LoginHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    db_ = std::make_shared<Database>("/app/data/email.db");
}

std::string LoginHandler::GenerateToken(int64_t user_id) const {
    // Generate JWT token with 24 hour expiration using kazuho-picojson traits
    auto token = ::jwt::create<::jwt::traits::kazuho_picojson>()
        .set_issuer("email-service")
        .set_type("JWT")
        .set_payload_claim("user_id", ::jwt::claim(std::to_string(user_id)))
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

    auto user = db_->AuthenticateUser(login_request.login, login_request.password);
    if (!user) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Invalid credentials")
        );
    }

    AuthToken token;
    token.token = GenerateToken(*user->id);
    token.user_id = *user->id;
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