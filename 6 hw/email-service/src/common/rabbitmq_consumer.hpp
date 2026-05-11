#pragma once

#include <string>
#include <vector>
#include <memory>

#include <userver/rabbitmq/consumer_component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/parse.hpp>

namespace email_service {

class EmailEventConsumer final : public userver::urabbitmq::ConsumerComponentBase {
public:
    static constexpr std::string_view kName{"email-event-consumer"};

    EmailEventConsumer(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context)
        : userver::urabbitmq::ConsumerComponentBase{config, context} {
        GetLogger()->info("EmailEventConsumer initialized");
    }

    // Get all consumed messages (for testing)
    std::vector<std::string> GetConsumedMessages() {
        auto storage = storage_.Lock();
        auto messages = *storage;
        return messages;
    }

    // Get consumed messages count
    size_t GetConsumedMessagesCount() {
        auto storage = storage_.Lock();
        return storage->size();
    }

    // Clear consumed messages (for testing)
    void ClearConsumedMessages() {
        auto storage = storage_.Lock();
        storage->clear();
    }

protected:
    void Process(std::string message) override {
        try {
            // Parse JSON message
            auto json = userver::formats::json::FromString(message);
            
            // Store message for testing
            {
                auto storage = storage_.Lock();
                storage->push_back(message);
            }
            
            // Log event
            std::string event_type = "Unknown";
            if (json.HasMember("event_type")) {
                event_type = json["event_type"].As<std::string>();
            }
            
            GetLogger()->info("Consumed event: type='{}', message_count={}", 
                            event_type, GetConsumedMessagesCount());
            
            // Process specific event types
            ProcessEvent(json);
            
        } catch (const std::exception& e) {
            GetLogger()->error("Error processing message: {}", e.what());
            throw;  // Return message to queue for retry
        }
    }

private:
    void ProcessEvent(const userver::formats::json::Value& event) {
        if (!event.HasMember("event_type")) {
            return;
        }
        
        std::string event_type = event["event_type"].As<std::string>();
        
        if (event_type == "UserCreated") {
            ProcessUserCreated(event);
        } else if (event_type == "UserUpdated") {
            ProcessUserUpdated(event);
        } else if (event_type == "FolderCreated") {
            ProcessFolderCreated(event);
        } else if (event_type == "MessageCreated") {
            ProcessMessageCreated(event);
        } else if (event_type == "MessageSent") {
            ProcessMessageSent(event);
        } else if (event_type == "MessageRead") {
            ProcessMessageRead(event);
        } else {
            GetLogger()->warn("Unknown event type: {}", event_type);
        }
    }

    void ProcessUserCreated(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string user_id = data["user_id"].As<std::string>();
            std::string login = data["login"].As<std::string>();
            
            GetLogger()->info("Processing UserCreated: user_id='{}', login='{}'", user_id, login);
            
            // Here you would implement actual business logic
            // For example: send welcome email, update analytics, etc.
        } catch (const std::exception& e) {
            GetLogger()->error("Error processing UserCreated: {}", e.what());
            throw;
        }
    }

    void ProcessUserUpdated(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string user_id = data["user_id"].As<std::string>();
            
            GetLogger()->info("Processing UserUpdated: user_id='{}'", user_id);
            
            // Here you would implement actual business logic
            // For example: update audit log, notify services, etc.
        } catch (const std::exception& e) {
            GetLogger()->error("Error processing UserUpdated: {}", e.what());
            throw;
        }
    }

    void ProcessFolderCreated(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string folder_id = data["folder_id"].As<std::string>();
            std::string name = data["name"].As<std::string>();
            
            GetLogger()->info("Processing FolderCreated: folder_id='{}', name='{}'", folder_id, name);
            
            // Here you would implement actual business logic
            // For example: update analytics, etc.
        } catch (const std::exception& e) {
            GetLogger()->error("Error processing FolderCreated: {}", e.what());
            throw;
        }
    }

    void ProcessMessageCreated(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string message_id = data["message_id"].As<std::string>();
            std::string subject = data["subject"].As<std::string>();
            
            GetLogger()->info("Processing MessageCreated: message_id='{}', subject='{}'", 
                            message_id, subject);
            
            // Here you would implement actual business logic
            // For example: index message for search, update analytics, etc.
        } catch (const std::exception& e) {
            GetLogger()->error("Error processing MessageCreated: {}", e.what());
            throw;
        }
    }

    void ProcessMessageSent(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string message_id = data["message_id"].As<std::string>();
            std::string recipient = data["recipient_email"].As<std::string>();
            
            GetLogger()->info("Processing MessageSent: message_id='{}', recipient='{}'", 
                            message_id, recipient);
            
            // Here you would implement actual business logic
            // For example: send notification to recipient, update analytics, etc.
        } catch (const std::exception& e) {
            GetLogger()->error("Error processing MessageSent: {}", e.what());
            throw;
        }
    }

    void ProcessMessageRead(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string message_id = data["message_id"].As<std::string>();
            
            GetLogger()->info("Processing MessageRead: message_id='{}'", message_id);
            
            // Here you would implement actual business logic
            // For example: update analytics, etc.
        } catch (const std::exception& e) {
            GetLogger()->error("Error processing MessageRead: {}", e.what());
            throw;
        }
    }

    userver::concurrent::Variable<std::vector<std::string>> storage_;
};

}  // namespace email_service
