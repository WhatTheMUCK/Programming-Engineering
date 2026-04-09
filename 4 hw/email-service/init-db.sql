-- ============================================================================
-- Email Service Database Initialization Script
-- This script is executed automatically when PostgreSQL container starts
-- Combines schema.sql and data.sql for initial setup
-- ============================================================================

-- ============================================================================
-- SCHEMA CREATION
-- ============================================================================

-- Drop existing tables if they exist (for clean re-initialization)
DROP TABLE IF EXISTS messages CASCADE;
DROP TABLE IF EXISTS folders CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- Create users table
CREATE TABLE users (
    id BIGSERIAL PRIMARY KEY,
    login VARCHAR(255) NOT NULL UNIQUE,
    email VARCHAR(255) NOT NULL UNIQUE,
    first_name VARCHAR(255) NOT NULL,
    last_name VARCHAR(255) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    
    CONSTRAINT chk_email_format CHECK (email LIKE '%@%'),
    CONSTRAINT chk_login_not_empty CHECK (LENGTH(TRIM(login)) > 0),
    CONSTRAINT chk_first_name_not_empty CHECK (LENGTH(TRIM(first_name)) > 0),
    CONSTRAINT chk_last_name_not_empty CHECK (LENGTH(TRIM(last_name)) > 0)
);

-- Create folders table
CREATE TABLE folders (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL,
    name VARCHAR(255) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    
    CONSTRAINT fk_folders_user FOREIGN KEY (user_id) 
        REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT chk_folder_name_not_empty CHECK (LENGTH(TRIM(name)) > 0),
    CONSTRAINT uq_user_folder_name UNIQUE (user_id, name)
);

-- Create messages table
CREATE TABLE messages (
    id BIGSERIAL PRIMARY KEY,
    folder_id BIGINT NOT NULL,
    sender_id BIGINT NOT NULL,
    recipient_email VARCHAR(255) NOT NULL,
    subject VARCHAR(500) NOT NULL,
    body TEXT NOT NULL,
    is_sent BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    
    CONSTRAINT fk_messages_folder FOREIGN KEY (folder_id) 
        REFERENCES folders(id) ON DELETE CASCADE,
    CONSTRAINT fk_messages_sender FOREIGN KEY (sender_id) 
        REFERENCES users(id) ON DELETE RESTRICT,
    CONSTRAINT chk_recipient_email_format CHECK (recipient_email LIKE '%@%'),
    CONSTRAINT chk_subject_not_empty CHECK (LENGTH(TRIM(subject)) > 0),
    CONSTRAINT chk_body_not_empty CHECK (LENGTH(TRIM(body)) > 0)
);

-- ============================================================================
-- INDEXES
-- ============================================================================

-- NOTE: PostgreSQL automatically creates indexes for PRIMARY KEY and UNIQUE constraints.
-- Therefore, we do NOT create explicit indexes for:
-- - users.login (covered by UNIQUE constraint)
-- - users.email (covered by UNIQUE constraint)
-- - folders(user_id, name) (covered by UNIQUE constraint)
-- Creating redundant indexes wastes disk space and slows down INSERT/UPDATE/DELETE operations.

CREATE INDEX idx_users_name_search ON users(first_name, last_name);
CREATE INDEX idx_users_name_pattern ON users(LOWER(first_name || ' ' || last_name) text_pattern_ops);
CREATE INDEX idx_folders_user_id ON folders(user_id);
CREATE INDEX idx_messages_folder_id ON messages(folder_id);
CREATE INDEX idx_messages_sender_id ON messages(sender_id);
CREATE INDEX idx_messages_created_at ON messages(created_at DESC);
CREATE INDEX idx_messages_folder_created ON messages(folder_id, created_at DESC);

-- ============================================================================
-- COMMENTS
-- ============================================================================

COMMENT ON TABLE users IS 'User accounts with authentication credentials';
COMMENT ON TABLE folders IS 'Email folders (Inbox, Sent, Drafts, etc.) belonging to users';
COMMENT ON TABLE messages IS 'Email messages stored within folders';

-- ============================================================================
-- STATISTICS
-- ============================================================================

ALTER TABLE users ALTER COLUMN login SET STATISTICS 1000;
ALTER TABLE users ALTER COLUMN email SET STATISTICS 1000;
ALTER TABLE folders ALTER COLUMN user_id SET STATISTICS 1000;
ALTER TABLE messages ALTER COLUMN folder_id SET STATISTICS 1000;
ALTER TABLE messages ALTER COLUMN created_at SET STATISTICS 1000;

-- ============================================================================
-- INITIAL DATA FOR DEVELOPMENT
-- ============================================================================
-- NOTE: This data is inserted here for convenience during development.
-- It matches the examples in openapi.yaml (john_doe user).
-- Tests will use a separate database (email_test_db) and will insert their own data.
-- For additional test data, use the data.sql file.

-- Insert sample user (matches openapi.yaml examples)
-- Password: "securePassword123" (SHA-256 hash)
INSERT INTO users (login, email, first_name, last_name, password_hash) VALUES
('john_doe', 'john@example.com', 'John', 'Doe', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d')
ON CONFLICT (login) DO NOTHING;

-- Insert sample folders for the demo user
INSERT INTO folders (user_id, name) VALUES
(1, 'Inbox'),
(1, 'Sent'),
(1, 'Drafts')
ON CONFLICT (user_id, name) DO NOTHING;

-- Insert sample message
INSERT INTO messages (folder_id, sender_id, recipient_email, subject, body, is_sent, created_at) VALUES
(1, 1, 'recipient@example.com', 'Welcome!', 'Welcome to the email service!', true, CURRENT_TIMESTAMP)
ON CONFLICT DO NOTHING;

-- ============================================================================
-- INITIALIZATION COMPLETE
-- ============================================================================
