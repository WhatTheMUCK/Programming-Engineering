#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace email_service {

struct User {
    std::string id;  // MongoDB ObjectId as string
    std::string login;
    std::string email;
    std::string first_name;
    std::string last_name;
    std::string password_hash;
    std::chrono::system_clock::time_point created_at;

    userver::formats::json::Value ToJson() const;
    static User FromJson(const userver::formats::json::Value& json);
};

struct Folder {
    std::string id;  // MongoDB ObjectId as string
    std::string user_id;  // MongoDB ObjectId as string (reference to user)
    std::string name;
    std::chrono::system_clock::time_point created_at;

    userver::formats::json::Value ToJson() const;
    static Folder FromJson(const userver::formats::json::Value& json);
};

struct Message {
    std::string id;  // MongoDB ObjectId as string
    std::string folder_id;  // MongoDB ObjectId as string (reference to folder)
    std::string sender_id;  // MongoDB ObjectId as string (reference to user)
    std::string recipient_email;
    std::string subject;
    std::string body;
    bool is_sent;
    std::chrono::system_clock::time_point created_at;

    userver::formats::json::Value ToJson() const;
    static Message FromJson(const userver::formats::json::Value& json);
};

struct AuthToken {
    std::string token;
    std::string user_id;  // MongoDB ObjectId as string
    std::chrono::system_clock::time_point expires_at;

    userver::formats::json::Value ToJson() const;
};

struct LoginRequest {
    std::string login;
    std::string password;

    static LoginRequest FromJson(const userver::formats::json::Value& json);
};

struct CreateUserRequest {
    std::string login;
    std::string email;
    std::string first_name;
    std::string last_name;
    std::string password;

    static CreateUserRequest FromJson(const userver::formats::json::Value& json);
};

struct CreateFolderRequest {
    std::string name;

    static CreateFolderRequest FromJson(const userver::formats::json::Value& json);
};

struct CreateMessageRequest {
    std::string recipient_email;
    std::string subject;
    std::string body;
    bool send_immediately;

    static CreateMessageRequest FromJson(const userver::formats::json::Value& json);
};

}  // namespace email_service