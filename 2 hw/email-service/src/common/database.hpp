#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <sqlite3.h>

#include "models.hpp"

namespace email_service {

class Database {
public:
    explicit Database(const std::string& db_path = "email.db");
    ~Database();

    // User operations
    std::optional<User> CreateUser(const CreateUserRequest& request);
    std::optional<User> FindUserByLogin(const std::string& login);
    std::optional<User> FindUserById(int64_t id);
    std::vector<User> SearchUsersByNameMask(const std::string& mask);
    std::optional<User> AuthenticateUser(const std::string& login, const std::string& password);

    // Folder operations
    std::optional<Folder> CreateFolder(int64_t user_id, const std::string& name);
    std::vector<Folder> ListFolders(int64_t user_id);
    std::optional<Folder> GetFolderById(int64_t folder_id);
    bool FolderBelongsToUser(int64_t folder_id, int64_t user_id);

    // Message operations
    std::optional<Message> CreateMessage(int64_t folder_id, int64_t sender_id, 
                                        const CreateMessageRequest& request);
    std::vector<Message> ListMessagesInFolder(int64_t folder_id);
    std::optional<Message> GetMessageById(int64_t message_id);
    bool MessageBelongsToUser(int64_t message_id, int64_t user_id);

    // Initialize database schema
    void InitSchema();

private:
    sqlite3* db_;
    
    std::string HashPassword(const std::string& password) const;
    bool VerifyPassword(const std::string& password, const std::string& hash) const;
    
    // Helper methods for SQLite operations
    bool ExecuteSQL(const std::string& sql);
    std::optional<int64_t> GetLastInsertId();
};

}  // namespace email_service
