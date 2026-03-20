#include "database.hpp"

#include <userver/crypto/hash.hpp>
#include <sstream>
#include <ctime>
#include <iostream>
#include <iomanip>

namespace email_service {

std::chrono::system_clock::time_point ParseDateTime(const std::string& datetime_str) {
    if (datetime_str.empty()) {
        return std::chrono::system_clock::now();
    }
    
    std::tm tm = {};
    std::istringstream ss(datetime_str);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    if (ss.fail()) {
        return std::chrono::system_clock::now();
    }
    
    std::time_t time_t_value = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(time_t_value);
}

Database::Database(const std::string& db_path) : db_(nullptr) {
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(db_)));
    }
    
    char* err_msg = nullptr;
    rc = sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::string error = err_msg ? err_msg : "Unknown error";
        sqlite3_free(err_msg);
        std::cerr << "Failed to enable WAL mode: " << error << std::endl;
    }
    
    sqlite3_busy_timeout(db_, 5000);
    
    InitSchema();
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

void Database::InitSchema() {
    std::cerr << "Initializing database schema..." << std::endl;
    
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            login TEXT UNIQUE NOT NULL,
            email TEXT UNIQUE NOT NULL,
            first_name TEXT NOT NULL,
            last_name TEXT NOT NULL,
            password_hash TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS folders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
            UNIQUE(user_id, name)
        );

        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            folder_id INTEGER NOT NULL,
            sender_id INTEGER NOT NULL,
            recipient_email TEXT NOT NULL,
            subject TEXT NOT NULL,
            body TEXT NOT NULL,
            is_sent INTEGER DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (folder_id) REFERENCES folders(id) ON DELETE CASCADE,
            FOREIGN KEY (sender_id) REFERENCES users(id)
        );

        CREATE INDEX IF NOT EXISTS idx_folders_user_id ON folders(user_id);
        CREATE INDEX IF NOT EXISTS idx_messages_folder_id ON messages(folder_id);
        CREATE INDEX IF NOT EXISTS idx_messages_sender_id ON messages(sender_id);
        CREATE INDEX IF NOT EXISTS idx_users_login ON users(login);
    )";
    
    bool result = ExecuteSQL(sql);
    if (result) {
        std::cerr << "Database schema initialized successfully" << std::endl;
    } else {
        std::cerr << "Failed to initialize database schema" << std::endl;
    }
}

bool Database::ExecuteSQL(const std::string& sql) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::string error = err_msg ? err_msg : "Unknown error";
        sqlite3_free(err_msg);
        std::cerr << "SQL Error: " << error << std::endl;
        std::cerr << "SQL: " << sql << std::endl;
        return false;
    }
    return true;
}

std::optional<int64_t> Database::GetLastInsertId() {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT last_insert_rowid()";
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    
    std::optional<int64_t> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int64(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return result;
}

std::string Database::HashPassword(const std::string& password) const {
    return userver::crypto::hash::Sha256(password);
}

bool Database::VerifyPassword(const std::string& password, const std::string& hash) const {
    return HashPassword(password) == hash;
}

std::optional<User> Database::CreateUser(const CreateUserRequest& request) {
    const char* sql = "INSERT INTO users (login, email, first_name, last_name, password_hash) "
                      "VALUES (?, ?, ?, ?, ?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    
    std::string password_hash = HashPassword(request.password);
    sqlite3_bind_text(stmt, 1, request.login.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, request.email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, request.first_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, request.last_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, password_hash.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        return std::nullopt;
    }
    
    auto user_id = GetLastInsertId();
    if (!user_id) {
        return std::nullopt;
    }
    
    return FindUserById(*user_id);
}

std::optional<User> Database::FindUserByLogin(const std::string& login) {
    const char* sql = "SELECT id, login, email, first_name, last_name, password_hash, created_at "
                      "FROM users WHERE login = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    
    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
    
    std::optional<User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        User user;
        user.id = sqlite3_column_int64(stmt, 0);
        user.login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.first_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user.last_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        user.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* created_at_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        user.created_at = ParseDateTime(created_at_str ? created_at_str : "");
        result = user;
    }
    
    sqlite3_finalize(stmt);
    return result;
}

std::optional<User> Database::FindUserById(int64_t id) {
    const char* sql = "SELECT id, login, email, first_name, last_name, password_hash, created_at "
                      "FROM users WHERE id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    
    sqlite3_bind_int64(stmt, 1, id);
    
    std::optional<User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        User user;
        user.id = sqlite3_column_int64(stmt, 0);
        user.login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.first_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user.last_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        user.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* created_at_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        user.created_at = ParseDateTime(created_at_str ? created_at_str : "");
        result = user;
    }
    
    sqlite3_finalize(stmt);
    return result;
}

std::vector<User> Database::SearchUsersByNameMask(const std::string& mask) {
    std::string search_pattern = "%" + mask + "%";
    const char* sql = "SELECT id, login, email, first_name, last_name, password_hash, created_at "
                      "FROM users WHERE first_name || ' ' || last_name LIKE ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }
    
    sqlite3_bind_text(stmt, 1, search_pattern.c_str(), -1, SQLITE_STATIC);
    
    std::vector<User> users;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        User user;
        user.id = sqlite3_column_int64(stmt, 0);
        user.login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.first_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user.last_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        user.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* created_at_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        user.created_at = ParseDateTime(created_at_str ? created_at_str : "");
        users.push_back(user);
    }
    
    sqlite3_finalize(stmt);
    return users;
}

std::optional<User> Database::AuthenticateUser(const std::string& login, const std::string& password) {
    auto user = FindUserByLogin(login);
    if (!user || !VerifyPassword(password, user->password_hash)) {
        return std::nullopt;
    }
    return user;
}

std::optional<Folder> Database::CreateFolder(int64_t user_id, const std::string& name) {
    const char* sql = "INSERT INTO folders (user_id, name) VALUES (?, ?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    
    sqlite3_bind_int64(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        return std::nullopt;
    }
    
    auto folder_id = GetLastInsertId();
    if (!folder_id) {
        return std::nullopt;
    }
    
    return GetFolderById(*folder_id);
}

std::vector<Folder> Database::ListFolders(int64_t user_id) {
    const char* sql = "SELECT id, user_id, name, created_at FROM folders WHERE user_id = ? ORDER BY created_at DESC";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }
    
    sqlite3_bind_int64(stmt, 1, user_id);
    
    std::vector<Folder> folders;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Folder folder;
        folder.id = sqlite3_column_int64(stmt, 0);
        folder.user_id = sqlite3_column_int64(stmt, 1);
        folder.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* created_at_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        folder.created_at = ParseDateTime(created_at_str ? created_at_str : "");
        folders.push_back(folder);
    }
    
    sqlite3_finalize(stmt);
    return folders;
}

std::optional<Folder> Database::GetFolderById(int64_t folder_id) {
    const char* sql = "SELECT id, user_id, name, created_at FROM folders WHERE id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    
    sqlite3_bind_int64(stmt, 1, folder_id);
    
    std::optional<Folder> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Folder folder;
        folder.id = sqlite3_column_int64(stmt, 0);
        folder.user_id = sqlite3_column_int64(stmt, 1);
        folder.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* created_at_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        folder.created_at = ParseDateTime(created_at_str ? created_at_str : "");
        result = folder;
    }
    
    sqlite3_finalize(stmt);
    return result;
}

bool Database::FolderBelongsToUser(int64_t folder_id, int64_t user_id) {
    auto folder = GetFolderById(folder_id);
    return folder && folder->user_id == user_id;
}

std::optional<Message> Database::CreateMessage(int64_t folder_id, int64_t sender_id,
                                               const CreateMessageRequest& request) {
    const char* sql = "INSERT INTO messages (folder_id, sender_id, recipient_email, subject, body, is_sent) "
                      "VALUES (?, ?, ?, ?, ?, ?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    
    sqlite3_bind_int64(stmt, 1, folder_id);
    sqlite3_bind_int64(stmt, 2, sender_id);
    sqlite3_bind_text(stmt, 3, request.recipient_email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, request.subject.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, request.body.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, request.send_immediately ? 1 : 0);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        return std::nullopt;
    }
    
    auto message_id = GetLastInsertId();
    if (!message_id) {
        return std::nullopt;
    }
    
    return GetMessageById(*message_id);
}

std::vector<Message> Database::ListMessagesInFolder(int64_t folder_id) {
    const char* sql = "SELECT id, folder_id, sender_id, recipient_email, subject, body, is_sent, created_at "
                      "FROM messages WHERE folder_id = ? ORDER BY created_at DESC";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }
    
    sqlite3_bind_int64(stmt, 1, folder_id);
    
    std::vector<Message> messages;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Message message;
        message.id = sqlite3_column_int64(stmt, 0);
        message.folder_id = sqlite3_column_int64(stmt, 1);
        message.sender_id = sqlite3_column_int64(stmt, 2);
        message.recipient_email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        message.subject = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        message.body = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        message.is_sent = sqlite3_column_int(stmt, 6) != 0;
        const char* created_at_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        message.created_at = ParseDateTime(created_at_str ? created_at_str : "");
        messages.push_back(message);
    }
    
    sqlite3_finalize(stmt);
    return messages;
}

std::optional<Message> Database::GetMessageById(int64_t message_id) {
    const char* sql = "SELECT id, folder_id, sender_id, recipient_email, subject, body, is_sent, created_at "
                      "FROM messages WHERE id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    
    sqlite3_bind_int64(stmt, 1, message_id);
    
    std::optional<Message> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Message message;
        message.id = sqlite3_column_int64(stmt, 0);
        message.folder_id = sqlite3_column_int64(stmt, 1);
        message.sender_id = sqlite3_column_int64(stmt, 2);
        message.recipient_email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        message.subject = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        message.body = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        message.is_sent = sqlite3_column_int(stmt, 6) != 0;
        const char* created_at_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        message.created_at = ParseDateTime(created_at_str ? created_at_str : "");
        result = message;
    }
    
    sqlite3_finalize(stmt);
    return result;
}

bool Database::MessageBelongsToUser(int64_t message_id, int64_t user_id) {
    const char* sql = "SELECT m.id FROM messages m "
                      "JOIN folders f ON m.folder_id = f.id "
                      "WHERE m.id = ? AND f.user_id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, message_id);
    sqlite3_bind_int64(stmt, 2, user_id);
    
    bool result = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return result;
}

}  // namespace email_service
