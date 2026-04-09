-- ============================================================================
-- Email Service Database Schema for PostgreSQL
-- Variant 9: Электронная почта (Email Service)
-- ============================================================================

-- Drop existing tables if they exist (for clean re-initialization)
DROP TABLE IF EXISTS messages CASCADE;
DROP TABLE IF EXISTS folders CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- ============================================================================
-- Table: users
-- Description: Stores user accounts with authentication credentials
-- ============================================================================
CREATE TABLE users (
    id BIGSERIAL PRIMARY KEY,
    login VARCHAR(255) NOT NULL UNIQUE,
    email VARCHAR(255) NOT NULL UNIQUE,
    first_name VARCHAR(255) NOT NULL,
    last_name VARCHAR(255) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    
    -- Constraints
    CONSTRAINT chk_email_format CHECK (email LIKE '%@%'),
    CONSTRAINT chk_login_not_empty CHECK (LENGTH(TRIM(login)) > 0),
    CONSTRAINT chk_first_name_not_empty CHECK (LENGTH(TRIM(first_name)) > 0),
    CONSTRAINT chk_last_name_not_empty CHECK (LENGTH(TRIM(last_name)) > 0)
);

-- ============================================================================
-- Table: folders
-- Description: Stores email folders (Inbox, Sent, Drafts, etc.) for each user
-- ============================================================================
CREATE TABLE folders (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL,
    name VARCHAR(255) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    
    -- Foreign key
    CONSTRAINT fk_folders_user FOREIGN KEY (user_id) 
        REFERENCES users(id) ON DELETE CASCADE,
    
    -- Constraints
    CONSTRAINT chk_folder_name_not_empty CHECK (LENGTH(TRIM(name)) > 0),
    
    -- Unique constraint: user cannot have duplicate folder names
    CONSTRAINT uq_user_folder_name UNIQUE (user_id, name)
);

-- ============================================================================
-- Table: messages
-- Description: Stores email messages within folders
-- ============================================================================
CREATE TABLE messages (
    id BIGSERIAL PRIMARY KEY,
    folder_id BIGINT NOT NULL,
    sender_id BIGINT NOT NULL,
    recipient_email VARCHAR(255) NOT NULL,
    subject VARCHAR(500) NOT NULL,
    body TEXT NOT NULL,
    is_sent BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    
    -- Foreign keys
    CONSTRAINT fk_messages_folder FOREIGN KEY (folder_id) 
        REFERENCES folders(id) ON DELETE CASCADE,
    CONSTRAINT fk_messages_sender FOREIGN KEY (sender_id) 
        REFERENCES users(id) ON DELETE RESTRICT,
    
    -- Constraints
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

-- Composite index on users first_name and last_name for name search
-- Rationale: Used in "Search user by name mask" API (WHERE first_name || ' ' || last_name ILIKE $1)
-- Expected usage: User search by partial name match
-- Note: PostgreSQL can use this index for LIKE/ILIKE queries with proper operator class
CREATE INDEX idx_users_name_search ON users(first_name, last_name);

-- Additional index for case-insensitive name search using text_pattern_ops
-- Rationale: Optimizes ILIKE queries with leading wildcards
-- This is essential for pattern matching with leading wildcards (%smith%)
CREATE INDEX idx_users_name_pattern ON users(LOWER(first_name || ' ' || last_name) text_pattern_ops);

-- Index on folders.user_id for listing user's folders
-- Rationale: Used in "List all folders" API (WHERE user_id = $1)
-- Expected usage: Every time user opens their folder list
-- Note: This is a foreign key, so indexing improves JOIN performance
CREATE INDEX idx_folders_user_id ON folders(user_id);

-- Index on messages.folder_id for listing messages in a folder
-- Rationale: Used in "List messages in folder" API (WHERE folder_id = $1)
-- Expected usage: Every time user opens a folder to view messages
-- Note: This is a foreign key, so indexing improves JOIN performance
CREATE INDEX idx_messages_folder_id ON messages(folder_id);

-- Index on messages.sender_id for joining messages with users
-- Rationale: Used when displaying sender information in message lists
-- Expected usage: JOIN operations between messages and users tables
CREATE INDEX idx_messages_sender_id ON messages(sender_id);

-- Index on messages.created_at for ordering messages by date
-- Rationale: Used in "List messages in folder" API (ORDER BY created_at DESC)
-- Expected usage: Every message list query, also useful for partitioning
-- Note: This index enables efficient time-range queries and is a candidate for partitioning
CREATE INDEX idx_messages_created_at ON messages(created_at DESC);

-- Composite index on messages for folder-based queries with date ordering
-- Rationale: Optimizes the most common query: get messages in folder ordered by date
-- Expected usage: Primary message listing query
CREATE INDEX idx_messages_folder_created ON messages(folder_id, created_at DESC);

-- ============================================================================
-- COMMENTS
-- ============================================================================

COMMENT ON TABLE users IS 'User accounts with authentication credentials';
COMMENT ON TABLE folders IS 'Email folders (Inbox, Sent, Drafts, etc.) belonging to users';
COMMENT ON TABLE messages IS 'Email messages stored within folders';

COMMENT ON COLUMN users.password_hash IS 'SHA-256 hash of user password';
COMMENT ON COLUMN messages.is_sent IS 'Flag indicating if message was sent externally';
COMMENT ON COLUMN messages.recipient_email IS 'Email address of the recipient (may be external)';

-- ============================================================================
-- STATISTICS
-- ============================================================================

-- Increase statistics target for frequently queried columns
-- This helps PostgreSQL query planner make better decisions
ALTER TABLE users ALTER COLUMN login SET STATISTICS 1000;
ALTER TABLE users ALTER COLUMN email SET STATISTICS 1000;
ALTER TABLE folders ALTER COLUMN user_id SET STATISTICS 1000;
ALTER TABLE messages ALTER COLUMN folder_id SET STATISTICS 1000;
ALTER TABLE messages ALTER COLUMN created_at SET STATISTICS 1000;

-- ============================================================================
-- END OF SCHEMA
-- ============================================================================
