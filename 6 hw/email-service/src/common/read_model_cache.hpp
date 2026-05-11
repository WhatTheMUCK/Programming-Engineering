#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>

#include <userver/components/loggable_component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>

#include "models.hpp"

namespace email_service {

/**
 * @brief Read Model Cache for CQRS pattern
 * 
 * This component maintains read models (denormalized views) that are synchronized
 * via events. It implements the Query side of CQRS pattern.
 * 
 * The cache stores:
 * - User read models (denormalized user data)
 * - Folder read models (denormalized folder data)
 * - Message read models (denormalized message data)
 * 
 * These are updated asynchronously when events are consumed.
 */
class ReadModelCache final : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName{"read-model-cache"};

    ReadModelCache(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

    // ============ User Read Model Operations ============

    /**
     * @brief Store or update user read model
     * @param user_id User identifier
     * @param user_data User data as JSON
     */
    void UpdateUserReadModel(const std::string& user_id,
                            const userver::formats::json::Value& user_data) {
        auto cache = user_cache_.Lock();
        (*cache)[user_id] = userver::formats::json::ToString(user_data);
        LOG_DEBUG() << "Updated user read model: user_id='" << user_id << "'";
    }

    /**
     * @brief Get user read model
     * @param user_id User identifier
     * @return User data as JSON string, or std::nullopt if not found
     */
    std::optional<std::string> GetUserReadModel(const std::string& user_id) {
        auto cache = user_cache_.Lock();
        auto it = cache->find(user_id);
        if (it != cache->end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Get all users from read model
     * @return Vector of user JSON strings
     */
    std::vector<std::string> GetAllUsersReadModel() {
        auto cache = user_cache_.Lock();
        std::vector<std::string> users;
        for (const auto& [user_id, user_data] : *cache) {
            users.push_back(user_data);
        }
        return users;
    }

    /**
     * @brief Invalidate user read model
     * @param user_id User identifier
     */
    void InvalidateUserReadModel(const std::string& user_id) {
        auto cache = user_cache_.Lock();
        cache->erase(user_id);
        LOG_DEBUG() << "Invalidated user read model: user_id='" << user_id << "'";
    }

    // ============ Folder Read Model Operations ============

    /**
     * @brief Store or update folder read model
     * @param folder_id Folder identifier
     * @param folder_data Folder data as JSON
     */
    void UpdateFolderReadModel(const std::string& folder_id,
                              const userver::formats::json::Value& folder_data) {
        auto cache = folder_cache_.Lock();
        (*cache)[folder_id] = userver::formats::json::ToString(folder_data);
        LOG_DEBUG() << "Updated folder read model: folder_id='" << folder_id << "'";
    }

    /**
     * @brief Get folder read model
     * @param folder_id Folder identifier
     * @return Folder data as JSON string, or std::nullopt if not found
     */
    std::optional<std::string> GetFolderReadModel(const std::string& folder_id) {
        auto cache = folder_cache_.Lock();
        auto it = cache->find(folder_id);
        if (it != cache->end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Get all folders from read model
     * @return Vector of folder JSON strings
     */
    std::vector<std::string> GetAllFoldersReadModel() {
        auto cache = folder_cache_.Lock();
        std::vector<std::string> folders;
        for (const auto& [folder_id, folder_data] : *cache) {
            folders.push_back(folder_data);
        }
        return folders;
    }

    /**
     * @brief Invalidate folder read model
     * @param folder_id Folder identifier
     */
    void InvalidateFolderReadModel(const std::string& folder_id) {
        auto cache = folder_cache_.Lock();
        cache->erase(folder_id);
        LOG_DEBUG() << "Invalidated folder read model: folder_id='" << folder_id << "'";
    }

    // ============ Message Read Model Operations ============

    /**
     * @brief Store or update message read model
     * @param message_id Message identifier
     * @param message_data Message data as JSON
     */
    void UpdateMessageReadModel(const std::string& message_id,
                               const userver::formats::json::Value& message_data) {
        auto cache = message_cache_.Lock();
        (*cache)[message_id] = userver::formats::json::ToString(message_data);
        LOG_DEBUG() << "Updated message read model: message_id='" << message_id << "'";
    }

    /**
     * @brief Get message read model
     * @param message_id Message identifier
     * @return Message data as JSON string, or std::nullopt if not found
     */
    std::optional<std::string> GetMessageReadModel(const std::string& message_id) {
        auto cache = message_cache_.Lock();
        auto it = cache->find(message_id);
        if (it != cache->end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Get all messages from read model
     * @return Vector of message JSON strings
     */
    std::vector<std::string> GetAllMessagesReadModel() {
        auto cache = message_cache_.Lock();
        std::vector<std::string> messages;
        for (const auto& [message_id, message_data] : *cache) {
            messages.push_back(message_data);
        }
        return messages;
    }

    /**
     * @brief Invalidate message read model
     * @param message_id Message identifier
     */
    void InvalidateMessageReadModel(const std::string& message_id) {
        auto cache = message_cache_.Lock();
        cache->erase(message_id);
        LOG_DEBUG() << "Invalidated message read model: message_id='" << message_id << "'";
    }

    // ============ Cache Statistics ============

    /**
     * @brief Get cache statistics
     * @return JSON object with cache sizes
     */
    userver::formats::json::Value GetCacheStats() {
        auto users = user_cache_.Lock();
        auto folders = folder_cache_.Lock();
        auto messages = message_cache_.Lock();

        userver::formats::json::ValueBuilder builder;
        builder["users_count"] = users->size();
        builder["folders_count"] = folders->size();
        builder["messages_count"] = messages->size();
        builder["total_count"] = users->size() + folders->size() + messages->size();

        return builder.ExtractValue();
    }

    /**
     * @brief Clear all read models
     */
    void ClearAll() {
        {
            auto cache = user_cache_.Lock();
            cache->clear();
        }
        {
            auto cache = folder_cache_.Lock();
            cache->clear();
        }
        {
            auto cache = message_cache_.Lock();
            cache->clear();
        }
        LOG_INFO() << "Cleared all read models";
    }

private:
    // Thread-safe storage for read models
    userver::concurrent::Variable<std::unordered_map<std::string, std::string>> user_cache_;
    userver::concurrent::Variable<std::unordered_map<std::string, std::string>> folder_cache_;
    userver::concurrent::Variable<std::unordered_map<std::string, std::string>> message_cache_;
};

}  // namespace email_service
