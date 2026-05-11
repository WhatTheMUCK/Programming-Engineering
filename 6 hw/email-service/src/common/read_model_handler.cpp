#include "read_model_handler.hpp"

#include <userver/formats/json.hpp>

namespace email_service {

userver::formats::json::Value ReadModelHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& request_body,
    userver::server::request::RequestContext&) const {
    
    std::string path = request.GetRequestPath();
    auto method = request.GetMethod();
    
    // GET /v1/read-models/stats
    if (path == "/v1/read-models/stats" && method == userver::server::http::HttpMethod::kGet) {
        return HandleGetStats();
    }
    
    // GET /v1/read-models/users
    if (path == "/v1/read-models/users" && method == userver::server::http::HttpMethod::kGet) {
        return HandleGetAllUsers();
    }
    
    // GET /v1/read-models/users/{user_id}
    if (path.find("/v1/read-models/users/") == 0 && method == userver::server::http::HttpMethod::kGet) {
        std::string user_id = path.substr(std::string("/v1/read-models/users/").length());
        return HandleGetUser(user_id);
    }
    
    // GET /v1/read-models/folders
    if (path == "/v1/read-models/folders" && method == userver::server::http::HttpMethod::kGet) {
        return HandleGetAllFolders();
    }
    
    // GET /v1/read-models/folders/{folder_id}
    if (path.find("/v1/read-models/folders/") == 0 && method == userver::server::http::HttpMethod::kGet) {
        std::string folder_id = path.substr(std::string("/v1/read-models/folders/").length());
        return HandleGetFolder(folder_id);
    }
    
    // GET /v1/read-models/messages
    if (path == "/v1/read-models/messages" && method == userver::server::http::HttpMethod::kGet) {
        return HandleGetAllMessages();
    }
    
    // GET /v1/read-models/messages/{message_id}
    if (path.find("/v1/read-models/messages/") == 0 && method == userver::server::http::HttpMethod::kGet) {
        std::string message_id = path.substr(std::string("/v1/read-models/messages/").length());
        return HandleGetMessage(message_id);
    }
    
    // Default: not found
    userver::formats::json::ValueBuilder builder;
    builder["error"] = "Not found";
    return builder.ExtractValue();
}

userver::formats::json::Value ReadModelHandler::HandleGetAllUsers() const {
    auto users = cache_.GetAllUsersReadModel();
    
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kArray);
    for (const auto& user_json : users) {
        builder.PushBack(userver::formats::json::FromString(user_json));
    }
    
    return builder.ExtractValue();
}

userver::formats::json::Value ReadModelHandler::HandleGetUser(const std::string& user_id) const {
    auto user_json = cache_.GetUserReadModel(user_id);
    
    if (!user_json) {
        userver::formats::json::ValueBuilder builder;
        builder["error"] = "User not found in read model";
        return builder.ExtractValue();
    }
    
    return userver::formats::json::FromString(*user_json);
}

userver::formats::json::Value ReadModelHandler::HandleGetAllFolders() const {
    auto folders = cache_.GetAllFoldersReadModel();
    
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kArray);
    for (const auto& folder_json : folders) {
        builder.PushBack(userver::formats::json::FromString(folder_json));
    }
    
    return builder.ExtractValue();
}

userver::formats::json::Value ReadModelHandler::HandleGetFolder(const std::string& folder_id) const {
    auto folder_json = cache_.GetFolderReadModel(folder_id);
    
    if (!folder_json) {
        userver::formats::json::ValueBuilder builder;
        builder["error"] = "Folder not found in read model";
        return builder.ExtractValue();
    }
    
    return userver::formats::json::FromString(*folder_json);
}

userver::formats::json::Value ReadModelHandler::HandleGetAllMessages() const {
    auto messages = cache_.GetAllMessagesReadModel();
    
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kArray);
    for (const auto& message_json : messages) {
        builder.PushBack(userver::formats::json::FromString(message_json));
    }
    
    return builder.ExtractValue();
}

userver::formats::json::Value ReadModelHandler::HandleGetMessage(const std::string& message_id) const {
    auto message_json = cache_.GetMessageReadModel(message_id);
    
    if (!message_json) {
        userver::formats::json::ValueBuilder builder;
        builder["error"] = "Message not found in read model";
        return builder.ExtractValue();
    }
    
    return userver::formats::json::FromString(*message_json);
}

userver::formats::json::Value ReadModelHandler::HandleGetStats() const {
    return cache_.GetCacheStats();
}

}  // namespace email_service
