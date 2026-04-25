#include "handlers.hpp"

#include <userver/components/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>

#include "../common/mongo_component.hpp"

namespace email_service::message {

std::string ExtractUserIdFromContext(const userver::server::request::RequestContext& context) {
    return context.GetData<std::string>("user_id");  // ObjectId as string
}

CreateMessageHandler::CreateMessageHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    auto& mongo_component = context.FindComponent<MongoComponent>("mongo-db");
    db_ = mongo_component.GetDatabase();
}

std::string CreateMessageHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    
    auto user_id = ExtractUserIdFromContext(context);
    
    auto folder_id = request.GetPathArg("folderId");  // ObjectId as string
    if (folder_id.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Folder ID is required")
        );
    }

    if (!db_->FolderBelongsToUser(folder_id, user_id)) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Access denied to this folder")
        );
    }
    
    auto request_body = userver::formats::json::FromString(request.RequestBody());
    
    if (!request_body.HasMember("recipient_email")) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Recipient email is required")
        );
    }
    if (!request_body.HasMember("subject")) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Subject is required")
        );
    }
    if (!request_body.HasMember("body")) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Body is required")
        );
    }
    
    std::string recipient_email = request_body["recipient_email"].As<std::string>();
    std::string subject = request_body["subject"].As<std::string>();
    std::string body = request_body["body"].As<std::string>();
    
    if (recipient_email.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Recipient email cannot be empty")
        );
    }
    if (subject.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Subject cannot be empty")
        );
    }
    if (body.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Body cannot be empty")
        );
    }
    
    if (recipient_email.find('@') == std::string::npos) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Invalid email format")
        );
    }
    
    auto create_request = CreateMessageRequest::FromJson(request_body);

    auto message = db_->CreateMessage(folder_id, user_id, create_request);
    if (!message) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Failed to create message")
        );
    }

    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    return userver::formats::json::ToString(message->ToJson());
}

ListMessagesHandler::ListMessagesHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    auto& mongo_component = context.FindComponent<MongoComponent>("mongo-db");
    db_ = mongo_component.GetDatabase();
}

std::string ListMessagesHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    
    auto user_id = ExtractUserIdFromContext(context);
    
    auto folder_id = request.GetPathArg("folderId");  // ObjectId as string
    if (folder_id.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Folder ID is required")
        );
    }

    if (!db_->FolderBelongsToUser(folder_id, user_id)) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Access denied to this folder")
        );
    }
    
    auto messages = db_->ListMessagesInFolder(folder_id);
    
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kArray);
    for (const auto& msg : messages) {
        builder.PushBack(msg.ToJson());
    }

    return userver::formats::json::ToString(builder.ExtractValue());
}

GetMessageHandler::GetMessageHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    auto& mongo_component = context.FindComponent<MongoComponent>("mongo-db");
    db_ = mongo_component.GetDatabase();
}

std::string GetMessageHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    
    auto user_id = ExtractUserIdFromContext(context);
    
    auto message_id = request.GetPathArg("messageId");  // ObjectId as string
    if (message_id.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Message ID is required")
        );
    }

    auto message = db_->GetMessageById(message_id);
    if (!message) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Message not found")
        );
    }

    if (!db_->MessageBelongsToUser(message_id, user_id)) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Access denied to this message")
        );
    }

    return userver::formats::json::ToString(message->ToJson());
}

void AppendMessageHandlers(userver::components::ComponentList& component_list) {
    component_list.Append<CreateMessageHandler>();
    component_list.Append<ListMessagesHandler>();
    component_list.Append<GetMessageHandler>();
}

}  // namespace email_service::message