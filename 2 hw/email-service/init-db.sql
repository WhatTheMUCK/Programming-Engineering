-- Initialize database schema
-- This script is automatically executed when the PostgreSQL container starts

-- Users table
CREATE TABLE IF NOT EXISTS users (
    id BIGSERIAL PRIMARY KEY,
    login VARCHAR(255) UNIQUE NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    first_name VARCHAR(255) NOT NULL,
    last_name VARCHAR(255) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Folders table
CREATE TABLE IF NOT EXISTS folders (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    name VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(user_id, name)
);

-- Messages table
CREATE TABLE IF NOT EXISTS messages (
    id BIGSERIAL PRIMARY KEY,
    folder_id BIGINT NOT NULL REFERENCES folders(id) ON DELETE CASCADE,
    sender_id BIGINT NOT NULL REFERENCES users(id),
    recipient_email VARCHAR(255) NOT NULL,
    subject VARCHAR(500) NOT NULL,
    body TEXT NOT NULL,
    is_sent BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Indexes for better performance
CREATE INDEX IF NOT EXISTS idx_folders_user_id ON folders(user_id);
CREATE INDEX IF NOT EXISTS idx_messages_folder_id ON messages(folder_id);
CREATE INDEX IF NOT EXISTS idx_messages_sender_id ON messages(sender_id);
CREATE INDEX IF NOT EXISTS idx_users_login ON users(login);

-- Insert sample data for testing
-- Password hashes are SHA256 of "securePassword123"
INSERT INTO users (login, email, first_name, last_name, password_hash)
VALUES
    ('john_doe', 'john@example.com', 'John', 'Doe', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d'),
    ('jane_smith', 'jane@example.com', 'Jane', 'Smith', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d')
ON CONFLICT (login) DO NOTHING;

-- Insert sample folders
INSERT INTO folders (user_id, name)
SELECT id, 'Inbox' FROM users WHERE login = 'john_doe'
ON CONFLICT (user_id, name) DO NOTHING;

INSERT INTO folders (user_id, name)
SELECT id, 'Sent' FROM users WHERE login = 'john_doe'
ON CONFLICT (user_id, name) DO NOTHING;

INSERT INTO folders (user_id, name)
SELECT id, 'Inbox' FROM users WHERE login = 'jane_smith'
ON CONFLICT (user_id, name) DO NOTHING;