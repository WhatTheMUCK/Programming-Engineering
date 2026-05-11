#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/oid.hpp>

#include "models.hpp"

namespace email_service {

// Helper function to convert JSON string to BSON document
inline bsoncxx::document::value FromJson(const std::string& json_str) {
    return bsoncxx::from_json(json_str);
}

class Database {
public:
    explicit Database(const std::string& mongo_uri);
    ~Database() = default;

    // User operations
    std::optional<User> CreateUser(const CreateUserRequest& request);
    std::optional<User> FindUserByLogin(const std::string& login);
    std::optional<User> FindUserById(const std::string& id);  // ObjectId string
    std::vector<User> SearchUsersByNameMask(const std::string& mask);
    std::optional<User> AuthenticateUser(const std::string& login, const std::string& password);

    // Folder operations
    std::optional<Folder> CreateFolder(const std::string& user_id, const std::string& name);  // ObjectId string
    std::vector<Folder> ListFolders(const std::string& user_id);  // ObjectId string
    std::optional<Folder> GetFolderById(const std::string& folder_id);  // ObjectId string
    bool FolderBelongsToUser(const std::string& folder_id, const std::string& user_id);  // ObjectId strings

    // Message operations
    std::optional<Message> CreateMessage(const std::string& folder_id, const std::string& sender_id,  // ObjectId strings
                                        const CreateMessageRequest& request);
    std::vector<Message> ListMessagesInFolder(const std::string& folder_id);  // ObjectId string
    std::optional<Message> GetMessageById(const std::string& message_id);  // ObjectId string
    bool MessageBelongsToUser(const std::string& message_id, const std::string& user_id);  // ObjectId strings

private:
    mongocxx::instance instance_;
    mongocxx::client client_;
    mongocxx::database db_;
    
    std::string HashPassword(const std::string& password) const;
    bool VerifyPassword(const std::string& password, const std::string& hash) const;
    
    // Helper methods for ObjectId conversion
    std::string ObjectIdToString(const bsoncxx::oid& oid) const;
    bsoncxx::oid StringToObjectId(const std::string& str) const;
    int64_t HexToInt64(const std::string& hex_str) const;
    
    // Helper methods for time conversion
    std::chrono::system_clock::time_point BsonDateToTimePoint(const bsoncxx::types::b_date& date) const;
    bsoncxx::types::b_date TimePointToBsonDate(const std::chrono::system_clock::time_point& tp) const;
    
    // Helper methods for domain model conversion
    User DocumentToUser(const bsoncxx::document::view& doc) const;
    Folder DocumentToFolder(const bsoncxx::document::view& doc) const;
    Message DocumentToMessage(const bsoncxx::document::view& doc) const;
};

}  // namespace email_service
