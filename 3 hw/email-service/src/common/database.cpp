#include "database.hpp"

#include <iostream>
#include <userver/crypto/hash.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/parameter_store.hpp>
#include <userver/storages/postgres/exceptions.hpp>

namespace email_service {

Database::Database(userver::storages::postgres::ClusterPtr pg_cluster)
    : pg_cluster_(std::move(pg_cluster)) {
    // No initialization needed - schema is managed by init-db.sql
}

std::string Database::HashPassword(const std::string& password) const {
    return userver::crypto::hash::Sha256(password);
}

bool Database::VerifyPassword(const std::string& password, const std::string& hash) const {
    return HashPassword(password) == hash;
}

// ============================================================================
// USER OPERATIONS
// ============================================================================

std::optional<User> Database::CreateUser(const CreateUserRequest& request) {
    try {
        std::string password_hash = HashPassword(request.password);
        
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO users (login, email, first_name, last_name, password_hash) "
            "VALUES ($1, $2, $3, $4, $5) "
            "RETURNING id, login, email, first_name, last_name, password_hash, created_at",
            request.login,
            request.email,
            request.first_name,
            request.last_name,
            password_hash
        );
        
        if (result.IsEmpty()) {
            std::cerr << "[CreateUser] Result is empty for login: " << request.login << std::endl;
            return std::nullopt;
        }
        
        auto row = result.AsSingleRow<User>(userver::storages::postgres::kRowTag);
        std::cerr << "[CreateUser] User created successfully: " << request.login << std::endl;
        return row;
        
    } catch (const userver::storages::postgres::UniqueViolation& e) {
        std::cerr << "[CreateUser] UniqueViolation for login: " << request.login << ", error: " << e.what() << std::endl;
        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "[CreateUser] Exception for login: " << request.login << ", error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<User> Database::FindUserByLogin(const std::string& login) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, login, email, first_name, last_name, password_hash, created_at "
            "FROM users WHERE login = $1",
            login
        );
        
        if (result.IsEmpty()) {
            return std::nullopt;
        }
        
        auto row = result.AsSingleRow<User>(userver::storages::postgres::kRowTag);
        return row;
        
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

std::optional<User> Database::FindUserById(int64_t id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, login, email, first_name, last_name, password_hash, created_at "
            "FROM users WHERE id = $1",
            id
        );
        
        if (result.IsEmpty()) {
            return std::nullopt;
        }
        
        auto row = result.AsSingleRow<User>(userver::storages::postgres::kRowTag);
        return row;
        
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

std::vector<User> Database::SearchUsersByNameMask(const std::string& mask) {
    try {
        // Add wildcards for ILIKE pattern matching
        std::string search_pattern = "%" + mask + "%";
        
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, login, email, first_name, last_name, password_hash, created_at "
            "FROM users "
            "WHERE LOWER(first_name || ' ' || last_name) LIKE LOWER($1) "
            "ORDER BY last_name, first_name",
            search_pattern
        );
        
        return result.AsContainer<std::vector<User>>(userver::storages::postgres::kRowTag);
        
    } catch (const std::exception& e) {
        return {};
    }
}

std::optional<User> Database::AuthenticateUser(const std::string& login, const std::string& password) {
    try {
        std::string password_hash = HashPassword(password);
        
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, login, email, first_name, last_name, password_hash, created_at "
            "FROM users WHERE login = $1 AND password_hash = $2",
            login,
            password_hash
        );
        
        if (result.IsEmpty()) {
            return std::nullopt;
        }
        
        auto row = result.AsSingleRow<User>(userver::storages::postgres::kRowTag);
        return row;
        
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

// ============================================================================
// FOLDER OPERATIONS
// ============================================================================

std::optional<Folder> Database::CreateFolder(int64_t user_id, const std::string& name) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO folders (user_id, name) "
            "VALUES ($1, $2) "
            "RETURNING id, user_id, name, created_at",
            user_id,
            name
        );
        
        if (result.IsEmpty()) {
            return std::nullopt;
        }
        
        auto row = result.AsSingleRow<Folder>(userver::storages::postgres::kRowTag);
        return row;
        
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

std::vector<Folder> Database::ListFolders(int64_t user_id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, user_id, name, created_at "
            "FROM folders "
            "WHERE user_id = $1 "
            "ORDER BY created_at ASC",
            user_id
        );
        
        return result.AsContainer<std::vector<Folder>>(userver::storages::postgres::kRowTag);
        
    } catch (const std::exception& e) {
        return {};
    }
}

std::optional<Folder> Database::GetFolderById(int64_t folder_id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, user_id, name, created_at "
            "FROM folders WHERE id = $1",
            folder_id
        );
        
        if (result.IsEmpty()) {
            return std::nullopt;
        }
        
        auto row = result.AsSingleRow<Folder>(userver::storages::postgres::kRowTag);
        return row;
        
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

bool Database::FolderBelongsToUser(int64_t folder_id, int64_t user_id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id FROM folders WHERE id = $1 AND user_id = $2",
            folder_id,
            user_id
        );
        
        return !result.IsEmpty();
        
    } catch (const std::exception& e) {
        return false;
    }
}

// ============================================================================
// MESSAGE OPERATIONS
// ============================================================================

std::optional<Message> Database::CreateMessage(int64_t folder_id, int64_t sender_id,
                                               const CreateMessageRequest& request) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO messages (folder_id, sender_id, recipient_email, subject, body, is_sent) "
            "VALUES ($1, $2, $3, $4, $5, $6) "
            "RETURNING id, folder_id, sender_id, recipient_email, subject, body, is_sent, created_at",
            folder_id,
            sender_id,
            request.recipient_email,
            request.subject,
            request.body,
            request.send_immediately
        );
        
        if (result.IsEmpty()) {
            std::cerr << "[CreateMessage] Result is empty for folder_id: " << folder_id << std::endl;
            return std::nullopt;
        }
        
        auto row = result.AsSingleRow<Message>(userver::storages::postgres::kRowTag);
        std::cerr << "[CreateMessage] Message created successfully for folder_id: " << folder_id << std::endl;
        return row;
        
    } catch (const std::exception& e) {
        std::cerr << "[CreateMessage] Exception for folder_id: " << folder_id << ", error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<Message> Database::ListMessagesInFolder(int64_t folder_id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, folder_id, sender_id, recipient_email, subject, body, is_sent, created_at "
            "FROM messages "
            "WHERE folder_id = $1 "
            "ORDER BY created_at DESC",
            folder_id
        );
        
        return result.AsContainer<std::vector<Message>>(userver::storages::postgres::kRowTag);
        
    } catch (const std::exception& e) {
        return {};
    }
}

std::optional<Message> Database::GetMessageById(int64_t message_id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, folder_id, sender_id, recipient_email, subject, body, is_sent, created_at "
            "FROM messages WHERE id = $1",
            message_id
        );
        
        if (result.IsEmpty()) {
            return std::nullopt;
        }
        
        auto row = result.AsSingleRow<Message>(userver::storages::postgres::kRowTag);
        return row;
        
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

bool Database::MessageBelongsToUser(int64_t message_id, int64_t user_id) {
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT m.id "
            "FROM messages m "
            "JOIN folders f ON m.folder_id = f.id "
            "WHERE m.id = $1 AND f.user_id = $2",
            message_id,
            user_id
        );
        
        return !result.IsEmpty();
        
    } catch (const std::exception& e) {
        return false;
    }
}

}  // namespace email_service
