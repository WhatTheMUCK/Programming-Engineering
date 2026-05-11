#pragma once

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#include <cstdlib>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "database.hpp"

namespace email_service {

class MongoComponent : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "mongo-db";

    MongoComponent(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context)
        : ComponentBase(config, context) {
        
        // Get URI from environment variable
        const char* env_uri = std::getenv("MONGO_URI");
        std::string mongo_uri;
        if (env_uri != nullptr && strlen(env_uri) > 0) {
            mongo_uri = env_uri;
        } else {
            // Default fallback
            mongo_uri = "mongodb://localhost:27017/email_db";
        }
        
        db_ = std::make_shared<Database>(mongo_uri);
    }

    std::shared_ptr<Database> GetDatabase() const {
        return db_;
    }

private:
    std::shared_ptr<Database> db_;
};

}  // namespace email_service
