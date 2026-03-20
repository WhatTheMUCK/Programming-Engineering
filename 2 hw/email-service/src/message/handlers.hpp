#pragma once

#include <memory>

#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>

#include "../common/database.hpp"
#include "../common/models.hpp"

namespace email_service::message {

// POST /api/v1/folders/{folderId}/messages - Create new message in folder
class CreateMessageHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-create-message";

    CreateMessageHandler(const userver::components::ComponentConfig& config,
                        const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

private:
    std::shared_ptr<Database> db_;
};

// GET /api/v1/folders/{folderId}/messages - List messages in folder
class ListMessagesHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-list-messages";

    ListMessagesHandler(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

private:
    std::shared_ptr<Database> db_;
};

// GET /api/v1/messages/{messageId} - Get message by ID
class GetMessageHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-get-message";

    GetMessageHandler(const userver::components::ComponentConfig& config,
                     const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

private:
    std::shared_ptr<Database> db_;
};

void AppendMessageHandlers(userver::components::ComponentList& component_list);

}  // namespace email_service::message