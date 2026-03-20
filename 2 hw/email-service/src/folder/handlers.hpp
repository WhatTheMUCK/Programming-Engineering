#pragma once

#include <memory>

#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>

#include "../common/database.hpp"
#include "../common/models.hpp"

namespace email_service::folder {

// POST /api/v1/folders - Create new folder
class CreateFolderHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-create-folder";

    CreateFolderHandler(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

private:
    std::shared_ptr<Database> db_;
};

// GET /api/v1/folders - List all folders for authenticated user
class ListFoldersHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-list-folders";

    ListFoldersHandler(const userver::components::ComponentConfig& config,
                      const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

private:
    std::shared_ptr<Database> db_;
};

void AppendFolderHandlers(userver::components::ComponentList& component_list);

}  // namespace email_service::folder