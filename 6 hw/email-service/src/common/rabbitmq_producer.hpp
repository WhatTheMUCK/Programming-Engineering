#pragma once

#include <string>
#include <memory>
#include <chrono>

#include <userver/utest/using_namespace_userver.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/rabbitmq.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/utils/uuid4.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "models.hpp"

namespace email_service {

class EmailEventProducer final : public components::LoggableComponentBase {
public:
    static constexpr std::string_view kName{"email-event-producer"};

    EmailEventProducer(const components::ComponentConfig& config,
                       const components::ComponentContext& context)
        : components::LoggableComponentBase{config, context},
          client_{context.FindComponent<components::RabbitMQ>(
                      config["rabbit_name"].As<std::string>())
                      .GetClient()} {
        const auto setup_deadline = engine::Deadline::FromDuration(std::chrono::seconds{2});

        auto admin_channel = client_->GetAdminChannel(setup_deadline);
        
        // Declare exchange
        admin_channel.DeclareExchange(exchange_, urabbitmq::Exchange::Type::kFanOut, setup_deadline);
        
        // Declare queue
        admin_channel.DeclareQueue(queue_, setup_deadline);
        
        // Bind queue to exchange
        admin_channel.BindQueue(exchange_, queue_, routing_key_, setup_deadline);
    }

    ~EmailEventProducer() override {
        try {
            auto admin_channel = client_->GetAdminChannel(
                engine::Deadline::FromDuration(std::chrono::seconds{1}));
            const auto teardown_deadline = engine::Deadline::FromDuration(std::chrono::seconds{2});
            
            admin_channel.RemoveQueue(queue_, teardown_deadline);
            admin_channel.RemoveExchange(exchange_, teardown_deadline);
        } catch (const std::exception& e) {
            // Cleanup error - log if needed
        }
    }

    // Publish UserCreated event
    void PublishUserCreated(const User& user) {
        formats::json::ValueBuilder builder{formats::json::Type::kObject};
        builder["event_id"] = utils::generators::GenerateUuid();
        builder["event_type"] = "UserCreated";
        builder["timestamp"] = std::chrono::system_clock::now();
        builder["version"] = "1.0";
        
        auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
        data["user_id"] = user.id;
        data["login"] = user.login;
        data["email"] = user.email;
        data["first_name"] = user.first_name;
        data["last_name"] = user.last_name;
        data["created_at"] = user.created_at;
        
        builder["data"] = data.ExtractValue();
        
        PublishEvent(builder.ExtractValue());
    }

    // Publish UserUpdated event
    void PublishUserUpdated(const User& user) {
        formats::json::ValueBuilder builder{formats::json::Type::kObject};
        builder["event_id"] = utils::generators::GenerateUuid();
        builder["event_type"] = "UserUpdated";
        builder["timestamp"] = std::chrono::system_clock::now();
        builder["version"] = "1.0";
        
        auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
        data["user_id"] = user.id;
        data["login"] = user.login;
        data["email"] = user.email;
        data["first_name"] = user.first_name;
        data["last_name"] = user.last_name;
        data["updated_at"] = std::chrono::system_clock::now();
        
        builder["data"] = data.ExtractValue();
        
        PublishEvent(builder.ExtractValue());
    }

    // Publish FolderCreated event
    void PublishFolderCreated(const Folder& folder) {
        formats::json::ValueBuilder builder{formats::json::Type::kObject};
        builder["event_id"] = utils::generators::GenerateUuid();
        builder["event_type"] = "FolderCreated";
        builder["timestamp"] = std::chrono::system_clock::now();
        builder["version"] = "1.0";
        
        auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
        data["folder_id"] = folder.id;
        data["user_id"] = folder.user_id;
        data["name"] = folder.name;
        data["created_at"] = folder.created_at;
        
        builder["data"] = data.ExtractValue();
        
        PublishEvent(builder.ExtractValue());
    }

    // Publish MessageCreated event
    void PublishMessageCreated(const Message& message) {
        formats::json::ValueBuilder builder{formats::json::Type::kObject};
        builder["event_id"] = utils::generators::GenerateUuid();
        builder["event_type"] = "MessageCreated";
        builder["timestamp"] = std::chrono::system_clock::now();
        builder["version"] = "1.0";
        
        auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
        data["message_id"] = message.id;
        data["folder_id"] = message.folder_id;
        data["sender_id"] = message.sender_id;
        data["recipient_email"] = message.recipient_email;
        data["subject"] = message.subject;
        data["body"] = message.body;
        data["is_sent"] = message.is_sent;
        data["created_at"] = message.created_at;
        
        builder["data"] = data.ExtractValue();
        
        PublishEvent(builder.ExtractValue());
    }

    // Publish MessageSent event
    void PublishMessageSent(const Message& message) {
        formats::json::ValueBuilder builder{formats::json::Type::kObject};
        builder["event_id"] = utils::generators::GenerateUuid();
        builder["event_type"] = "MessageSent";
        builder["timestamp"] = std::chrono::system_clock::now();
        builder["version"] = "1.0";
        
        auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
        data["message_id"] = message.id;
        data["folder_id"] = message.folder_id;
        data["sender_id"] = message.sender_id;
        data["recipient_email"] = message.recipient_email;
        data["subject"] = message.subject;
        data["sent_at"] = std::chrono::system_clock::now();
        
        builder["data"] = data.ExtractValue();
        
        PublishEvent(builder.ExtractValue());
    }

    // Publish MessageRead event
    void PublishMessageRead(const std::string& message_id, const std::string& user_id,
                           const std::string& folder_id) {
        formats::json::ValueBuilder builder{formats::json::Type::kObject};
        builder["event_id"] = utils::generators::GenerateUuid();
        builder["event_type"] = "MessageRead";
        builder["timestamp"] = std::chrono::system_clock::now();
        builder["version"] = "1.0";
        
        auto data = formats::json::ValueBuilder{formats::json::Type::kObject};
        data["message_id"] = message_id;
        data["folder_id"] = folder_id;
        data["user_id"] = user_id;
        data["read_at"] = std::chrono::system_clock::now();
        
        builder["data"] = data.ExtractValue();
        
        PublishEvent(builder.ExtractValue());
    }

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Email Event Producer component for RabbitMQ
additionalProperties: false
properties:
    rabbit_name:
        type: string
        description: name of RabbitMQ client component
        )");
    }

private:
    void PublishEvent(const formats::json::Value& event) {
        try {
            const std::string message = formats::json::ToString(event);
            
            client_->PublishReliable(
                exchange_,
                routing_key_,
                message,
                urabbitmq::MessageType::kTransient,
                engine::Deadline::FromDuration(std::chrono::seconds{2})
            );
        } catch (const std::exception& e) {
            throw;
        }
    }

    const urabbitmq::Exchange exchange_{"email-events"};
    const urabbitmq::Queue queue_{"email-events-queue"};
    const std::string routing_key_ = "email-routing-key";

    std::shared_ptr<urabbitmq::Client> client_;
};

}  // namespace email_service
