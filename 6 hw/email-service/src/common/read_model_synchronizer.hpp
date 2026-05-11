#pragma once

#include <string>
#include <memory>

#include <userver/rabbitmq/consumer_component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/parse.hpp>

#include "read_model_cache.hpp"

namespace email_service {

/**
 * @brief Read Model Synchronizer - Event Consumer for CQRS
 * 
 * This component consumes events from RabbitMQ and updates the read models
 * in the ReadModelCache. It implements the event-driven synchronization
 * of the Query side in CQRS pattern.
 * 
 * When an event is consumed:
 * 1. Parse the event JSON
 * 2. Extract the event type and data
 * 3. Update the corresponding read model in the cache
 * 4. Log the synchronization
 */
class ReadModelSynchronizer final : public userver::urabbitmq::ConsumerComponentBase {
public:
    static constexpr std::string_view kName{"read-model-synchronizer"};

    ReadModelSynchronizer(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context)
        : userver::urabbitmq::ConsumerComponentBase{config, context},
          cache_(context.FindComponent<ReadModelCache>()) {
        GetLogger()->info("ReadModelSynchronizer initialized");
    }

protected:
    void Process(std::string message) override {
        try {
            // Parse JSON message
            auto json = userver::formats::json::FromString(message);
            
            // Extract event type
            std::string event_type = "Unknown";
            if (json.HasMember("event_type")) {
                event_type = json["event_type"].As<std::string>();
            }
            
            GetLogger()->info("Synchronizing read model for event: type='{}'", event_type);
            
            // Process specific event types and update read models
            SynchronizeReadModel(json);
            
        } catch (const std::exception& e) {
            GetLogger()->error("Error synchronizing read model: {}", e.what());
            throw;  // Return message to queue for retry
        }
    }

private:
    ReadModelCache& cache_;

    void SynchronizeReadModel(const userver::formats::json::Value& event) {
        if (!event.HasMember("event_type")) {
            return;
        }
        
        std::string event_type = event["event_type"].As<std::string>();
        
        if (event_type == "UserCreated") {
            SynchronizeUserCreated(event);
        } else if (event_type == "UserUpdated") {
            SynchronizeUserUpdated(event);
        } else if (event_type == "FolderCreated") {
            SynchronizeFolderCreated(event);
        } else if (event_type == "MessageCreated") {
            SynchronizeMessageCreated(event);
        } else if (event_type == "MessageSent") {
            SynchronizeMessageSent(event);
        } else if (event_type == "MessageRead") {
            SynchronizeMessageRead(event);
        } else {
            GetLogger()->warn("Unknown event type for read model sync: {}", event_type);
        }
    }

    void SynchronizeUserCreated(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string user_id = data["user_id"].As<std::string>();
            
            // Update read model with user data
            cache_.UpdateUserReadModel(user_id, data);
            
            GetLogger()->info("Synchronized UserCreated read model: user_id='{}'", user_id);
        } catch (const std::exception& e) {
            GetLogger()->error("Error synchronizing UserCreated: {}", e.what());
            throw;
        }
    }

    void SynchronizeUserUpdated(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string user_id = data["user_id"].As<std::string>();
            
            // Update read model with user data
            cache_.UpdateUserReadModel(user_id, data);
            
            GetLogger()->info("Synchronized UserUpdated read model: user_id='{}'", user_id);
        } catch (const std::exception& e) {
            GetLogger()->error("Error synchronizing UserUpdated: {}", e.what());
            throw;
        }
    }

    void SynchronizeFolderCreated(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string folder_id = data["folder_id"].As<std::string>();
            
            // Update read model with folder data
            cache_.UpdateFolderReadModel(folder_id, data);
            
            GetLogger()->info("Synchronized FolderCreated read model: folder_id='{}'", folder_id);
        } catch (const std::exception& e) {
            GetLogger()->error("Error synchronizing FolderCreated: {}", e.what());
            throw;
        }
    }

    void SynchronizeMessageCreated(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string message_id = data["message_id"].As<std::string>();
            
            // Update read model with message data
            cache_.UpdateMessageReadModel(message_id, data);
            
            GetLogger()->info("Synchronized MessageCreated read model: message_id='{}'", message_id);
        } catch (const std::exception& e) {
            GetLogger()->error("Error synchronizing MessageCreated: {}", e.what());
            throw;
        }
    }

    void SynchronizeMessageSent(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string message_id = data["message_id"].As<std::string>();
            
            // Update read model with message data (status changed to sent)
            cache_.UpdateMessageReadModel(message_id, data);
            
            GetLogger()->info("Synchronized MessageSent read model: message_id='{}'", message_id);
        } catch (const std::exception& e) {
            GetLogger()->error("Error synchronizing MessageSent: {}", e.what());
            throw;
        }
    }

    void SynchronizeMessageRead(const userver::formats::json::Value& event) {
        try {
            auto data = event["data"];
            std::string message_id = data["message_id"].As<std::string>();
            
            // Update read model with message data (status changed to read)
            cache_.UpdateMessageReadModel(message_id, data);
            
            GetLogger()->info("Synchronized MessageRead read model: message_id='{}'", message_id);
        } catch (const std::exception& e) {
            GetLogger()->error("Error synchronizing MessageRead: {}", e.what());
            throw;
        }
    }
};

}  // namespace email_service
