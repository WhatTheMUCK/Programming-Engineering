#pragma once

#include <string>
#include <memory>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/formats/json/value.hpp>

#include "read_model_cache.hpp"

namespace email_service {

/**
 * @brief HTTP Handler for Read Model Queries
 * 
 * Exposes endpoints to query the read models maintained by ReadModelCache.
 * This implements the Query side of CQRS pattern.
 * 
 * Endpoints:
 * - GET /v1/read-models/users - Get all users from read model
 * - GET /v1/read-models/users/{user_id} - Get specific user from read model
 * - GET /v1/read-models/folders - Get all folders from read model
 * - GET /v1/read-models/folders/{folder_id} - Get specific folder from read model
 * - GET /v1/read-models/messages - Get all messages from read model
 * - GET /v1/read-models/messages/{message_id} - Get specific message from read model
 * - GET /v1/read-models/stats - Get read model cache statistics
 */
class ReadModelHandler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-read-model";

    ReadModelHandler(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context)
        : HttpHandlerJsonBase(config, context),
          cache_(context.FindComponent<ReadModelCache>()) {}

    userver::formats::json::Value HandleRequestJsonThrow(
        const userver::server::http::HttpRequest& request,
        const userver::formats::json::Value& request_body,
        userver::server::request::RequestContext&) const override;

private:
    ReadModelCache& cache_;

    // Handler methods for different endpoints
    userver::formats::json::Value HandleGetAllUsers() const;
    userver::formats::json::Value HandleGetUser(const std::string& user_id) const;
    userver::formats::json::Value HandleGetAllFolders() const;
    userver::formats::json::Value HandleGetFolder(const std::string& folder_id) const;
    userver::formats::json::Value HandleGetAllMessages() const;
    userver::formats::json::Value HandleGetMessage(const std::string& message_id) const;
    userver::formats::json::Value HandleGetStats() const;
};

}  // namespace email_service
