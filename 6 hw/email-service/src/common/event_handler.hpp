#pragma once

#include <string>
#include <vector>

#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_response.hpp>

#include "rabbitmq_producer.hpp"
#include "rabbitmq_consumer.hpp"

namespace email_service {

class EventHandler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "event-handler";

    EventHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context)
        : userver::server::handlers::HttpHandlerJsonBase{config, context},
          producer_{context.FindComponent<EmailEventProducer>()},
          consumer_{context.FindComponent<EmailEventConsumer>()} {}

    ~EventHandler() override = default;

    userver::formats::json::Value HandleRequestJsonThrow(
        const userver::server::http::HttpRequest& request,
        const userver::formats::json::Value& request_json,
        userver::server::request::RequestContext&) const override {
        
        request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
        
        if (request.GetMethod() == userver::server::http::HttpMethod::kGet) {
            return HandleGetConsumedEvents();
        } else if (request.GetMethod() == userver::server::http::HttpMethod::kPost) {
            return HandlePublishTestEvent(request_json);
        } else if (request.GetMethod() == userver::server::http::HttpMethod::kDelete) {
            return HandleClearConsumedEvents();
        } else {
            request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed);
            userver::formats::json::ValueBuilder error_builder{userver::formats::json::Type::kObject};
            error_builder["error"] = "Method not allowed";
            return error_builder.ExtractValue();
        }
    }

    static userver::yaml_config::Schema GetStaticConfigSchema() {
        return userver::yaml_config::MergeSchemas<userver::server::handlers::HttpHandlerJsonBase>(R"(
type: object
description: Event handler for testing RabbitMQ producer and consumer
additionalProperties: false
        )");
    }

private:
    userver::formats::json::Value HandleGetConsumedEvents() const {
        userver::formats::json::ValueBuilder response{userver::formats::json::Type::kObject};
        
        auto messages = consumer_.GetConsumedMessages();
        
        userver::formats::json::ValueBuilder messages_array{userver::formats::json::Type::kArray};
        for (const auto& msg : messages) {
            try {
                auto json_msg = userver::formats::json::FromString(msg);
                messages_array.PushBack(json_msg);
            } catch (const std::exception& e) {
                GetLogger()->warn("Failed to parse message: {}", e.what());
            }
        }
        
        response["count"] = messages.size();
        response["messages"] = messages_array.ExtractValue();
        
        return response.ExtractValue();
    }

    userver::formats::json::Value HandlePublishTestEvent(
        const userver::formats::json::Value& request_json) const {
        
        userver::formats::json::ValueBuilder response{userver::formats::json::Type::kObject};
        
        try {
            if (!request_json.HasMember("event_type")) {
                response["error"] = "Missing required field: event_type";
                response["success"] = false;
                return response.ExtractValue();
            }
            
            std::string event_type = request_json["event_type"].As<std::string>();
            
            // Create a test user for UserCreated event
            if (event_type == "UserCreated") {
                User test_user;
                test_user.id = "507f1f77bcf86cd799439011";
                test_user.login = "test_user";
                test_user.email = "test@example.com";
                test_user.first_name = "Test";
                test_user.last_name = "User";
                test_user.created_at = std::chrono::system_clock::now();
                
                producer_.PublishUserCreated(test_user);
                
                response["success"] = true;
                response["message"] = "UserCreated event published";
                response["event_type"] = "UserCreated";
            }
            // Create a test folder for FolderCreated event
            else if (event_type == "FolderCreated") {
                Folder test_folder;
                test_folder.id = "507f1f77bcf86cd799439012";
                test_folder.user_id = "507f1f77bcf86cd799439011";
                test_folder.name = "Test Folder";
                test_folder.created_at = std::chrono::system_clock::now();
                
                producer_.PublishFolderCreated(test_folder);
                
                response["success"] = true;
                response["message"] = "FolderCreated event published";
                response["event_type"] = "FolderCreated";
            }
            // Create a test message for MessageCreated event
            else if (event_type == "MessageCreated") {
                Message test_message;
                test_message.id = "507f1f77bcf86cd799439013";
                test_message.folder_id = "507f1f77bcf86cd799439012";
                test_message.sender_id = "507f1f77bcf86cd799439011";
                test_message.recipient_email = "recipient@example.com";
                test_message.subject = "Test Subject";
                test_message.body = "Test message body";
                test_message.is_sent = false;
                test_message.created_at = std::chrono::system_clock::now();
                
                producer_.PublishMessageCreated(test_message);
                
                response["success"] = true;
                response["message"] = "MessageCreated event published";
                response["event_type"] = "MessageCreated";
            }
            // Publish MessageSent event
            else if (event_type == "MessageSent") {
                Message test_message;
                test_message.id = "507f1f77bcf86cd799439013";
                test_message.folder_id = "507f1f77bcf86cd799439012";
                test_message.sender_id = "507f1f77bcf86cd799439011";
                test_message.recipient_email = "recipient@example.com";
                test_message.subject = "Test Subject";
                test_message.body = "Test message body";
                test_message.is_sent = true;
                test_message.created_at = std::chrono::system_clock::now();
                
                producer_.PublishMessageSent(test_message);
                
                response["success"] = true;
                response["message"] = "MessageSent event published";
                response["event_type"] = "MessageSent";
            }
            // Publish MessageRead event
            else if (event_type == "MessageRead") {
                producer_.PublishMessageRead(
                    "507f1f77bcf86cd799439013",
                    "507f1f77bcf86cd799439011",
                    "507f1f77bcf86cd799439012"
                );
                
                response["success"] = true;
                response["message"] = "MessageRead event published";
                response["event_type"] = "MessageRead";
            }
            else {
                response["error"] = "Unknown event type: " + event_type;
                response["success"] = false;
            }
        } catch (const std::exception& e) {
            response["error"] = std::string("Error publishing event: ") + e.what();
            response["success"] = false;
            GetLogger()->error("Error in HandlePublishTestEvent: {}", e.what());
        }
        
        return response.ExtractValue();
    }

    userver::formats::json::Value HandleClearConsumedEvents() const {
        userver::formats::json::ValueBuilder response{userver::formats::json::Type::kObject};
        
        try {
            consumer_.ClearConsumedMessages();
            response["success"] = true;
            response["message"] = "Consumed events cleared";
        } catch (const std::exception& e) {
            response["error"] = std::string("Error clearing events: ") + e.what();
            response["success"] = false;
            GetLogger()->error("Error in HandleClearConsumedEvents: {}", e.what());
        }
        
        return response.ExtractValue();
    }

    EmailEventProducer& producer_;
    EmailEventConsumer& consumer_;
};

}  // namespace email_service
