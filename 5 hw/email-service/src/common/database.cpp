#include "database.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <userver/crypto/hash.hpp>

namespace email_service {

Database::Database(const std::string& mongo_uri)
    : client_(mongocxx::uri{mongo_uri}) {
    
    // Extract database name from URI, default to email_db
    mongocxx::uri uri(mongo_uri);
    std::string db_name = uri.database();
    if (db_name.empty()) {
        db_name = "email_db";
    }
    db_ = client_[db_name];
    
    std::cerr << "[Database] Connected to MongoDB successfully, uri: " << mongo_uri
              << ", database: " << db_name << std::endl;
}

std::string Database::HashPassword(const std::string& password) const {
    return userver::crypto::hash::Sha256(password);
}

bool Database::VerifyPassword(const std::string& password, const std::string& hash) const {
    return HashPassword(password) == hash;
}

std::string Database::ObjectIdToString(const bsoncxx::oid& oid) const {
    return oid.to_string();
}

// Helper function to convert hex string (MongoDB ObjectId) to positive int64_t
// Uses first 4 bytes (timestamp, 8 hex chars) + last 3 bytes (counter, 6 hex chars)
int64_t Database::HexToInt64(const std::string& hex_str) const {
    // ObjectId: 4 bytes timestamp (8 hex) + 5 bytes machine + 3 bytes counter (6 hex)
    // Take first 8 hex chars (timestamp) + last 6 hex chars (counter) = 14 chars
    std::string timestamp = hex_str.substr(0, 8);
    std::string counter = hex_str.substr(hex_str.length() - 6);
    std::string hex7bytes = timestamp + counter;
    
    // Convert hex to uint64_t
    std::uint64_t result = 0;
    for (char c : hex7bytes) {
        result *= 16;
        if (c >= '0' && c <= '9') {
            result += c - '0';
        } else if (c >= 'a' && c <= 'f') {
            result += 10 + (c - 'a');
        } else if (c >= 'A' && c <= 'F') {
            result += 10 + (c - 'A');
        }
    }
    
    // Apply positive mask (clear sign bit)
    return static_cast<int64_t>(result & 0x7FFFFFFFFFFFFFFFULL);
}

bsoncxx::oid Database::StringToObjectId(const std::string& str) const {
    return bsoncxx::oid(str);
}

std::chrono::system_clock::time_point Database::BsonDateToTimePoint(const bsoncxx::types::b_date& date) const {
    return std::chrono::system_clock::time_point(
        std::chrono::milliseconds(date.to_int64())
    );
}

bsoncxx::types::b_date Database::TimePointToBsonDate(const std::chrono::system_clock::time_point& tp) const {
    auto duration = tp.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return bsoncxx::types::b_date{std::chrono::milliseconds{millis}};
}

User Database::DocumentToUser(const bsoncxx::document::view& doc) const {
    User user;
    user.id = ObjectIdToString(doc["_id"].get_oid().value);  // ObjectId as string
    user.login = doc["login"].get_utf8().value.to_string();
    user.email = doc["email"].get_utf8().value.to_string();
    user.first_name = doc["firstName"].get_utf8().value.to_string();
    user.last_name = doc["lastName"].get_utf8().value.to_string();
    user.password_hash = doc["passwordHash"].get_utf8().value.to_string();
    user.created_at = BsonDateToTimePoint(doc["createdAt"].get_date());
    return user;
}

Folder Database::DocumentToFolder(const bsoncxx::document::view& doc) const {
    Folder folder;
    folder.id = ObjectIdToString(doc["_id"].get_oid().value);  // ObjectId as string
    folder.user_id = ObjectIdToString(doc["userId"].get_oid().value);  // ObjectId as string
    folder.name = doc["name"].get_utf8().value.to_string();
    folder.created_at = BsonDateToTimePoint(doc["createdAt"].get_date());
    return folder;
}

Message Database::DocumentToMessage(const bsoncxx::document::view& doc) const {
    Message message;
    message.id = ObjectIdToString(doc["_id"].get_oid().value);  // ObjectId as string
    message.folder_id = ObjectIdToString(doc["folderId"].get_oid().value);  // ObjectId as string
    
    auto sender_view = doc["sender"].get_document().view();
    message.sender_id = ObjectIdToString(sender_view["_id"].get_oid().value);  // ObjectId as string
    
    message.recipient_email = doc["recipientEmail"].get_utf8().value.to_string();
    message.subject = doc["subject"].get_utf8().value.to_string();
    message.body = doc["body"].get_utf8().value.to_string();
    message.is_sent = doc["isSent"].get_bool().value;
    message.created_at = BsonDateToTimePoint(doc["createdAt"].get_date());
    return message;
}

// ============================================================================
// USER OPERATIONS
// ============================================================================

std::optional<User> Database::CreateUser(const CreateUserRequest& request) {
    try {
        auto collection = db_["users"];
        
        auto existing_by_login = collection.find_one(FromJson(R"({"login": ")" + request.login + R"("})"));
        if (existing_by_login) {
            std::cerr << "[CreateUser] User already exists with login: " << request.login << std::endl;
            return std::nullopt;
        }

        auto existing_by_email = collection.find_one(FromJson(R"({"email": ")" + request.email + R"("})"));
        std::cerr << "[CreateUser] Duplicate email pre-check for '" << request.email
                  << "': " << (existing_by_email ? "FOUND" : "NOT FOUND") << std::endl;
        if (existing_by_email) {
            std::cerr << "[CreateUser] User already exists with email: " << request.email << std::endl;
            return std::nullopt;
        }
        
        std::string password_hash = HashPassword(request.password);
        auto now = std::chrono::system_clock::now();
        
        auto doc = FromJson(R"({
            "login": ")" + request.login + R"(",
            "email": ")" + request.email + R"(",
            "firstName": ")" + request.first_name + R"(",
            "lastName": ")" + request.last_name + R"(",
            "passwordHash": ")" + password_hash + R"(",
            "createdAt": {"$date": )" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()) + R"(}
        })");
        
        decltype(collection.insert_one(doc.view())) result;
        try {
            result = collection.insert_one(doc.view());
        } catch (const std::exception& e) {
            std::cerr << "[CreateUser] Insert failed for login: " << request.login
                      << ", email: " << request.email << ", error: " << e.what() << std::endl;
            return std::nullopt;
        }

        if (!result) {
            std::cerr << "[CreateUser] Failed to insert user: " << request.login << std::endl;
            return std::nullopt;
        }
        
        // Get the inserted ID and construct user object directly
        auto inserted_id = result->inserted_id().get_oid().value;
        std::string oid_str = ObjectIdToString(inserted_id);
        
        User user;
        user.id = oid_str;  // ObjectId as string
        user.login = request.login;
        user.email = request.email;
        user.first_name = request.first_name;
        user.last_name = request.last_name;
        user.password_hash = password_hash;
        user.created_at = now;
        
        std::cerr << "[CreateUser] User created successfully: " << request.login << ", id: " << oid_str << std::endl;
        return user;
        
    } catch (const std::exception& e) {
        std::cerr << "[CreateUser] Exception for login: " << request.login << ", error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<User> Database::FindUserByLogin(const std::string& login) {
    try {
        auto collection = db_["users"];
        auto result = collection.find_one(FromJson(R"({"login": ")" + login + R"("})"));
        
        if (!result) {
            return std::nullopt;
        }
        
        return DocumentToUser(result->view());
        
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

std::optional<User> Database::FindUserById(const std::string& id) {
    try {
        auto collection = db_["users"];
        // Direct lookup by ObjectId
        auto result = collection.find_one(FromJson(R"({"_id": {"$oid": ")" + id + R"("}})"));

        if (!result) {
            return std::nullopt;
        }

        return DocumentToUser(result->view());

    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

std::vector<User> Database::SearchUsersByNameMask(const std::string& mask) {
    try {
        auto collection = db_["users"];
        
        std::string pattern = ".*" + mask + ".*";
        
        auto result = collection.find(FromJson(R"({
            "$or": [
                {"firstName": {"$regex": ")" + pattern + R"(", "$options": "i"}},
                {"lastName": {"$regex": ")" + pattern + R"(", "$options": "i"}}
            ]
        })"));
        
        std::vector<User> users;
        for (auto&& doc : result) {
            users.push_back(DocumentToUser(doc));
        }
        
        return users;
        
    } catch (const std::exception& e) {
        return {};
    }
}

std::optional<User> Database::AuthenticateUser(const std::string& login, const std::string& password) {
    try {
        std::string password_hash = HashPassword(password);
        
        auto collection = db_["users"];
        auto result = collection.find_one(FromJson(R"({
            "login": ")" + login + R"(",
            "passwordHash": ")" + password_hash + R"("
        })"));
        
        if (!result) {
            return std::nullopt;
        }
        
        return DocumentToUser(result->view());
        
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

// ============================================================================
// FOLDER OPERATIONS
// ============================================================================

std::optional<Folder> Database::CreateFolder(const std::string& user_id, const std::string& name) {
    try {
        auto collection = db_["folders"];
        auto now = std::chrono::system_clock::now();
        
        // Use ObjectId directly for userId reference
        auto doc = FromJson(R"({
            "userId": {"$oid": ")" + user_id + R"("},
            "name": ")" + name + R"(",
            "createdAt": {"$date": )" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()) + R"(}
        })");
        
        auto result = collection.insert_one(doc.view());
        
        if (!result) {
            std::cerr << "[CreateFolder] Failed to insert folder for user_id: " << user_id << std::endl;
            return std::nullopt;
        }
        
        auto inserted_id = result->inserted_id().get_oid().value;
        auto new_folder = collection.find_one(FromJson(R"({"_id": {"$oid": ")" + ObjectIdToString(inserted_id) + R"("}})"));
        
        if (!new_folder) {
            std::cerr << "[CreateFolder] Failed to retrieve inserted folder" << std::endl;
            return std::nullopt;
        }
        
        return DocumentToFolder(new_folder->view());
        
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

std::vector<Folder> Database::ListFolders(const std::string& user_id) {
    try {
        auto collection = db_["folders"];
        
        // Direct lookup by user ObjectId
        auto result = collection.find(FromJson(R"({"userId": {"$oid": ")" + user_id + R"("}})"));
        
        std::vector<Folder> folders;
        for (auto&& doc : result) {
            folders.push_back(DocumentToFolder(doc));
        }
        
        return folders;
        
    } catch (const std::exception& e) {
        return {};
    }
}

std::optional<Folder> Database::GetFolderById(const std::string& folder_id) {
    try {
        auto collection = db_["folders"];
        // Direct lookup by ObjectId
        auto result = collection.find_one(FromJson(R"({"_id": {"$oid": ")" + folder_id + R"("}})"));

        if (!result) {
            return std::nullopt;
        }

        return DocumentToFolder(result->view());

    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

bool Database::FolderBelongsToUser(const std::string& folder_id, const std::string& user_id) {
    try {
        auto folder = GetFolderById(folder_id);
        return folder && folder->user_id == user_id;

    } catch (const std::exception& e) {
        return false;
    }
}

// ============================================================================
// MESSAGE OPERATIONS
// ============================================================================

std::optional<Message> Database::CreateMessage(const std::string& folder_id, const std::string& sender_id,
                                               const CreateMessageRequest& request) {
    try {
        auto collection = db_["messages"];
        auto now = std::chrono::system_clock::now();
        
        auto users_collection = db_["users"];
        
        // Find sender by ObjectId
        auto sender_doc = users_collection.find_one(FromJson(R"({"_id": {"$oid": ")" + sender_id + R"("}})"));
        if (!sender_doc) {
            std::cerr << "[CreateMessage] Sender not found with id: " << sender_id << std::endl;
            return std::nullopt;
        }

        auto sender_view = sender_doc->view();
        
        // Use ObjectId directly for sender embedded document
        std::string sender_json = R"({
            "_id": {"$oid": ")" + sender_id + R"("},
            "login": ")" + sender_view["login"].get_utf8().value.to_string() + R"(",
            "email": ")" + sender_view["email"].get_utf8().value.to_string() + R"(",
            "firstName": ")" + sender_view["firstName"].get_utf8().value.to_string() + R"(",
            "lastName": ")" + sender_view["lastName"].get_utf8().value.to_string() + R"("
        })";
        
        // Use ObjectId for folderId reference
        auto doc = FromJson(R"({
            "folderId": {"$oid": ")" + folder_id + R"("},
            "sender": )" + sender_json + R"(,
            "recipientEmail": ")" + request.recipient_email + R"(",
            "subject": ")" + request.subject + R"(",
            "body": ")" + request.body + R"(",
            "isSent": )" + (request.send_immediately ? "true" : "false") + R"(,
            "createdAt": {"$date": )" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()) + R"(}
        })");
        
        auto result = collection.insert_one(doc.view());
        
        if (!result) {
            std::cerr << "[CreateMessage] Failed to insert message for folder_id: " << folder_id << std::endl;
            return std::nullopt;
        }
        
        auto inserted_id = result->inserted_id().get_oid().value;
        auto new_message = collection.find_one(FromJson(R"({"_id": {"$oid": ")" + ObjectIdToString(inserted_id) + R"("}})"));
        
        if (!new_message) {
            std::cerr << "[CreateMessage] Failed to retrieve inserted message" << std::endl;
            return std::nullopt;
        }
        
        std::cerr << "[CreateMessage] Message created successfully for folder_id: " << folder_id << std::endl;
        return DocumentToMessage(new_message->view());
        
    } catch (const std::exception& e) {
        std::cerr << "[CreateMessage] Exception for folder_id: " << folder_id << ", error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<Message> Database::ListMessagesInFolder(const std::string& folder_id) {
    try {
        auto collection = db_["messages"];
        
        // Direct lookup by folder ObjectId
        auto result = collection.find(FromJson(R"({"folderId": {"$oid": ")" + folder_id + R"("}})"));
        
        std::vector<Message> messages;
        for (auto&& doc : result) {
            messages.push_back(DocumentToMessage(doc));
        }
        
        return messages;
        
    } catch (const std::exception& e) {
        return {};
    }
}

std::optional<Message> Database::GetMessageById(const std::string& message_id) {
    try {
        auto collection = db_["messages"];
        // Direct lookup by ObjectId
        auto result = collection.find_one(FromJson(R"({"_id": {"$oid": ")" + message_id + R"("}})"));

        if (!result) {
            return std::nullopt;
        }

        return DocumentToMessage(result->view());

    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

bool Database::MessageBelongsToUser(const std::string& message_id, const std::string& user_id) {
    try {
        auto message = GetMessageById(message_id);
        if (!message) {
            return false;
        }

        return FolderBelongsToUser(message->folder_id, user_id);

    } catch (const std::exception& e) {
        return false;
    }
}

}  // namespace email_service
