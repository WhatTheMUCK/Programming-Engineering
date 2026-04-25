#include "handlers.hpp"

#include <userver/components/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>

#include "../common/mongo_component.hpp"
#include "../common/cache_component.hpp"

namespace email_service::folder {

std::string ExtractUserIdFromContext(const userver::server::request::RequestContext& context) {
    return context.GetData<std::string>("user_id");  // ObjectId as string
}

CreateFolderHandler::CreateFolderHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    auto& mongo_component = context.FindComponent<MongoComponent>("mongo-db");
    db_ = mongo_component.GetDatabase();
    
    // Get cache component if available
    try {
        cache_ = &context.FindComponent<CacheComponent>("cache");
    } catch (...) {
        cache_ = nullptr;
    }
}

std::string CreateFolderHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    
    auto user_id = ExtractUserIdFromContext(context);
    
    auto request_body = userver::formats::json::FromString(request.RequestBody());
    
    if (!request_body.HasMember("name")) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Name is required")
        );
    }
    
    std::string name = request_body["name"].As<std::string>();
    if (name.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Name cannot be empty")
        );
    }
    
    auto create_request = CreateFolderRequest::FromJson(request_body);

    auto folder = db_->CreateFolder(user_id, create_request.name);
    if (!folder) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
        return userver::formats::json::ToString(
            userver::formats::json::MakeObject("error", "Folder already exists or invalid data")
        );
    }

    // Invalidate cache for this user's folder list
    if (cache_) {
        cache_->GetFolderCache()->Invalidate("folder:list:" + user_id);
    }

    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    return userver::formats::json::ToString(folder->ToJson());
}

ListFoldersHandler::ListFoldersHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    auto& mongo_component = context.FindComponent<MongoComponent>("mongo-db");
    db_ = mongo_component.GetDatabase();
    
    // Get cache component if available
    try {
        cache_ = &context.FindComponent<CacheComponent>("cache");
    } catch (...) {
        cache_ = nullptr;
    }
}

std::string ListFoldersHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    
    auto user_id = ExtractUserIdFromContext(context);
    
    // Check cache first
    std::string cache_key = "folder:list:" + user_id;
    if (cache_) {
        auto cached_folders = cache_->GetFolderCache()->Get(cache_key);
        if (cached_folders) {
            cache_->GetFolderCacheStats().RecordHit();
            return *cached_folders;
        }
        cache_->GetFolderCacheStats().RecordMiss();
    }
    
    auto folders = db_->ListFolders(user_id);
    
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kArray);
    for (const auto& folder : folders) {
        builder.PushBack(folder.ToJson());
    }

    auto result = userver::formats::json::ToString(builder.ExtractValue());

    // Cache the result
    if (cache_) {
        cache_->GetFolderCache()->Put(cache_key, result);
    }

    return result;
}

void AppendFolderHandlers(userver::components::ComponentList& component_list) {
    component_list.Append<CreateFolderHandler>();
    component_list.Append<ListFoldersHandler>();
}

}  // namespace email_service::folder