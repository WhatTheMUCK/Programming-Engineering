#include "models.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/utils/datetime.hpp>

namespace email_service {

// User
userver::formats::json::Value User::ToJson() const {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = id;  // ObjectId as string
    builder["login"] = login;
    builder["email"] = email;
    builder["first_name"] = first_name;
    builder["last_name"] = last_name;
    builder["created_at"] = userver::utils::datetime::Timestring(created_at);
    return builder.ExtractValue();
}

User User::FromJson(const userver::formats::json::Value& json) {
    User user;
    if (json.HasMember("id")) {
        user.id = json["id"].As<std::string>();
    }
    user.login = json["login"].As<std::string>();
    user.email = json["email"].As<std::string>();
    user.first_name = json["first_name"].As<std::string>();
    user.last_name = json["last_name"].As<std::string>();
    if (json.HasMember("password")) {
        user.password_hash = json["password"].As<std::string>();
    }
    return user;
}

// Folder
userver::formats::json::Value Folder::ToJson() const {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = id;  // ObjectId as string
    builder["user_id"] = user_id;  // ObjectId as string (reference to user)
    builder["name"] = name;
    builder["created_at"] = userver::utils::datetime::Timestring(created_at);
    return builder.ExtractValue();
}

Folder Folder::FromJson(const userver::formats::json::Value& json) {
    Folder folder;
    if (json.HasMember("id")) {
        folder.id = json["id"].As<std::string>();
    }
    if (json.HasMember("user_id")) {
        folder.user_id = json["user_id"].As<std::string>();
    }
    folder.name = json["name"].As<std::string>();
    return folder;
}

// Message
userver::formats::json::Value Message::ToJson() const {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = id;  // ObjectId as string
    builder["folder_id"] = folder_id;  // ObjectId as string (reference to folder)
    builder["sender_id"] = sender_id;  // ObjectId as string (reference to user)
    builder["recipient_email"] = recipient_email;
    builder["subject"] = subject;
    builder["body"] = body;
    builder["is_sent"] = is_sent;
    builder["created_at"] = userver::utils::datetime::Timestring(created_at);
    return builder.ExtractValue();
}

Message Message::FromJson(const userver::formats::json::Value& json) {
    Message message;
    if (json.HasMember("id")) {
        message.id = json["id"].As<std::string>();
    }
    if (json.HasMember("folder_id")) {
        message.folder_id = json["folder_id"].As<std::string>();
    }
    if (json.HasMember("sender_id")) {
        message.sender_id = json["sender_id"].As<std::string>();
    }
    message.recipient_email = json["recipient_email"].As<std::string>();
    message.subject = json["subject"].As<std::string>();
    message.body = json["body"].As<std::string>();
    message.is_sent = json.HasMember("is_sent") ? json["is_sent"].As<bool>() : false;
    return message;
}

// AuthToken
userver::formats::json::Value AuthToken::ToJson() const {
    userver::formats::json::ValueBuilder builder;
    builder["token"] = token;
    builder["user_id"] = user_id;  // ObjectId as string
    builder["expires_at"] = userver::utils::datetime::Timestring(expires_at);
    return builder.ExtractValue();
}

// LoginRequest
LoginRequest LoginRequest::FromJson(const userver::formats::json::Value& json) {
    LoginRequest request;
    if (!json.HasMember("login")) {
        throw std::runtime_error("Missing required field: login");
    }
    if (!json.HasMember("password")) {
        throw std::runtime_error("Missing required field: password");
    }
    request.login = json["login"].As<std::string>();
    request.password = json["password"].As<std::string>();
    return request;
}

// CreateUserRequest
CreateUserRequest CreateUserRequest::FromJson(const userver::formats::json::Value& json) {
    CreateUserRequest request;
    if (!json.HasMember("login")) {
        throw std::runtime_error("Missing required field: login");
    }
    if (!json.HasMember("email")) {
        throw std::runtime_error("Missing required field: email");
    }
    if (!json.HasMember("first_name")) {
        throw std::runtime_error("Missing required field: first_name");
    }
    if (!json.HasMember("last_name")) {
        throw std::runtime_error("Missing required field: last_name");
    }
    if (!json.HasMember("password")) {
        throw std::runtime_error("Missing required field: password");
    }
    request.login = json["login"].As<std::string>();
    request.email = json["email"].As<std::string>();
    request.first_name = json["first_name"].As<std::string>();
    request.last_name = json["last_name"].As<std::string>();
    request.password = json["password"].As<std::string>();
    return request;
}

// CreateFolderRequest
CreateFolderRequest CreateFolderRequest::FromJson(const userver::formats::json::Value& json) {
    CreateFolderRequest request;
    if (!json.HasMember("name")) {
        throw std::runtime_error("Missing required field: name");
    }
    request.name = json["name"].As<std::string>();
    return request;
}

// CreateMessageRequest
CreateMessageRequest CreateMessageRequest::FromJson(const userver::formats::json::Value& json) {
    CreateMessageRequest request;
    if (!json.HasMember("recipient_email")) {
        throw std::runtime_error("Missing required field: recipient_email");
    }
    if (!json.HasMember("subject")) {
        throw std::runtime_error("Missing required field: subject");
    }
    if (!json.HasMember("body")) {
        throw std::runtime_error("Missing required field: body");
    }
    request.recipient_email = json["recipient_email"].As<std::string>();
    request.subject = json["subject"].As<std::string>();
    request.body = json["body"].As<std::string>();
    request.send_immediately = json.HasMember("send_immediately")
        ? json["send_immediately"].As<bool>()
        : false;
    return request;
}

}  // namespace email_service