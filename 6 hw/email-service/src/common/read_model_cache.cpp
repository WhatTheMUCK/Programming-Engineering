#include "read_model_cache.hpp"

namespace email_service {

ReadModelCache::ReadModelCache(const userver::components::ComponentConfig& config,
                               const userver::components::ComponentContext& context)
    : userver::components::LoggableComponentBase(config, context) {
}

}  // namespace email_service
