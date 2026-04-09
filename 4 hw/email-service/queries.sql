-- ============================================================================
-- Email Service SQL Queries
-- Variant 9: Электронная почта (Email Service)
-- All queries for the 8 required API operations
-- ============================================================================

-- ============================================================================
-- USER OPERATIONS
-- ============================================================================

-- ----------------------------------------------------------------------------
-- 1. Create new user (Создание нового пользователя)
-- API: POST /api/v1/users
-- Parameters: $1=login, $2=email, $3=first_name, $4=last_name, $5=password_hash
-- Returns: user_id
-- ----------------------------------------------------------------------------
INSERT INTO users (login, email, first_name, last_name, password_hash)
VALUES ($1, $2, $3, $4, $5)
RETURNING id, login, email, first_name, last_name, created_at;

-- Example:
-- INSERT INTO users (login, email, first_name, last_name, password_hash)
-- VALUES ('john.doe', 'john.doe@example.com', 'John', 'Doe', 'hashed_password_here')
-- RETURNING id, login, email, first_name, last_name, created_at;


-- ----------------------------------------------------------------------------
-- 2. Find user by login (Поиск пользователя по логину)
-- API: GET /api/v1/users/by-login?login=...
-- Parameters: $1=login
-- Returns: user details (without password_hash)
-- ----------------------------------------------------------------------------
SELECT id, login, email, first_name, last_name, created_at
FROM users
WHERE login = $1;

-- Example:
-- SELECT id, login, email, first_name, last_name, created_at
-- FROM users
-- WHERE login = 'alice.smith';


-- ----------------------------------------------------------------------------
-- 3. Search user by name mask (Поиск пользователя по маске имя и фамилии)
-- API: GET /api/v1/users/search?mask=...
-- Parameters: $1=name_mask (e.g., '%alice%', '%smith%')
-- Returns: list of matching users
-- Note: Uses ILIKE for case-insensitive search
-- ----------------------------------------------------------------------------
SELECT id, login, email, first_name, last_name, created_at
FROM users
WHERE LOWER(first_name || ' ' || last_name) LIKE LOWER($1)
ORDER BY last_name, first_name;

-- Alternative with ILIKE (PostgreSQL-specific, case-insensitive):
SELECT id, login, email, first_name, last_name, created_at
FROM users
WHERE first_name ILIKE $1 OR last_name ILIKE $1 OR (first_name || ' ' || last_name) ILIKE $1
ORDER BY last_name, first_name;

-- Example:
-- SELECT id, login, email, first_name, last_name, created_at
-- FROM users
-- WHERE (first_name || ' ' || last_name) ILIKE '%smith%'
-- ORDER BY last_name, first_name;


-- ----------------------------------------------------------------------------
-- Auxiliary: Authenticate user (for login endpoint)
-- API: POST /api/v1/auth/login
-- Parameters: $1=login, $2=password_hash
-- Returns: user_id if credentials match, NULL otherwise
-- ----------------------------------------------------------------------------
SELECT id, login, email, first_name, last_name, created_at
FROM users
WHERE login = $1 AND password_hash = $2;

-- Example:
-- SELECT id, login, email, first_name, last_name, created_at
-- FROM users
-- WHERE login = 'alice.smith' AND password_hash = 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f';


-- ============================================================================
-- FOLDER OPERATIONS
-- ============================================================================

-- ----------------------------------------------------------------------------
-- 4. Create new folder (Создание новой почтовой папки)
-- API: POST /api/v1/folders
-- Parameters: $1=user_id, $2=folder_name
-- Returns: folder_id
-- Note: Requires authentication, user_id from JWT token
-- ----------------------------------------------------------------------------
INSERT INTO folders (user_id, name)
VALUES ($1, $2)
RETURNING id, user_id, name, created_at;

-- Example:
-- INSERT INTO folders (user_id, name)
-- VALUES (1, 'Important')
-- RETURNING id, user_id, name, created_at;


-- ----------------------------------------------------------------------------
-- 5. Get list of all folders (Получение перечня всех папок)
-- API: GET /api/v1/folders
-- Parameters: $1=user_id (from JWT token)
-- Returns: list of folders for the authenticated user
-- ----------------------------------------------------------------------------
SELECT id, user_id, name, created_at
FROM folders
WHERE user_id = $1
ORDER BY created_at ASC;

-- Example:
-- SELECT id, user_id, name, created_at
-- FROM folders
-- WHERE user_id = 1
-- ORDER BY created_at ASC;


-- ----------------------------------------------------------------------------
-- Auxiliary: Check folder ownership
-- Used to verify that a folder belongs to the authenticated user
-- Parameters: $1=folder_id, $2=user_id
-- Returns: folder details if user owns it, NULL otherwise
-- ----------------------------------------------------------------------------
SELECT id, user_id, name, created_at
FROM folders
WHERE id = $1 AND user_id = $2;

-- Example:
-- SELECT id, user_id, name, created_at
-- FROM folders
-- WHERE id = 1 AND user_id = 1;


-- ============================================================================
-- MESSAGE OPERATIONS
-- ============================================================================

-- ----------------------------------------------------------------------------
-- 6. Create new message in folder (Создание нового письма в папке)
-- API: POST /api/v1/folders/{folderId}/messages
-- Parameters: $1=folder_id, $2=sender_id, $3=recipient_email, $4=subject, $5=body, $6=is_sent
-- Returns: message_id
-- Note: Requires authentication and folder ownership verification
-- ----------------------------------------------------------------------------
INSERT INTO messages (folder_id, sender_id, recipient_email, subject, body, is_sent)
VALUES ($1, $2, $3, $4, $5, $6)
RETURNING id, folder_id, sender_id, recipient_email, subject, body, is_sent, created_at;

-- Example:
-- INSERT INTO messages (folder_id, sender_id, recipient_email, subject, body, is_sent)
-- VALUES (1, 1, 'recipient@example.com', 'Test Subject', 'Test message body', true)
-- RETURNING id, folder_id, sender_id, recipient_email, subject, body, is_sent, created_at;


-- ----------------------------------------------------------------------------
-- 7. Get all messages in folder (Получение всех писем в папке)
-- API: GET /api/v1/folders/{folderId}/messages
-- Parameters: $1=folder_id
-- Returns: list of messages in the folder, ordered by date (newest first)
-- Note: Requires folder ownership verification
-- ----------------------------------------------------------------------------
SELECT m.id, m.folder_id, m.sender_id, m.recipient_email, m.subject, m.body, m.is_sent, m.created_at,
       u.login as sender_login, u.first_name as sender_first_name, u.last_name as sender_last_name
FROM messages m
JOIN users u ON m.sender_id = u.id
WHERE m.folder_id = $1
ORDER BY m.created_at DESC;

-- Simplified version without JOIN (if sender details not needed):
SELECT id, folder_id, sender_id, recipient_email, subject, body, is_sent, created_at
FROM messages
WHERE folder_id = $1
ORDER BY created_at DESC;

-- Example:
-- SELECT m.id, m.folder_id, m.sender_id, m.recipient_email, m.subject, m.body, m.is_sent, m.created_at,
--        u.login as sender_login, u.first_name as sender_first_name, u.last_name as sender_last_name
-- FROM messages m
-- JOIN users u ON m.sender_id = u.id
-- WHERE m.folder_id = 1
-- ORDER BY m.created_at DESC;


-- ----------------------------------------------------------------------------
-- 8. Get message by ID (Получение письма по коду)
-- API: GET /api/v1/messages/{messageId}
-- Parameters: $1=message_id
-- Returns: message details with sender information
-- Note: Requires ownership verification (message must be in user's folder)
-- ----------------------------------------------------------------------------
SELECT m.id, m.folder_id, m.sender_id, m.recipient_email, m.subject, m.body, m.is_sent, m.created_at,
       u.login as sender_login, u.first_name as sender_first_name, u.last_name as sender_last_name,
       f.name as folder_name, f.user_id as folder_owner_id
FROM messages m
JOIN users u ON m.sender_id = u.id
JOIN folders f ON m.folder_id = f.id
WHERE m.id = $1;

-- Example:
-- SELECT m.id, m.folder_id, m.sender_id, m.recipient_email, m.subject, m.body, m.is_sent, m.created_at,
--        u.login as sender_login, u.first_name as sender_first_name, u.last_name as sender_last_name,
--        f.name as folder_name, f.user_id as folder_owner_id
-- FROM messages m
-- JOIN users u ON m.sender_id = u.id
-- JOIN folders f ON m.folder_id = f.id
-- WHERE m.id = 1;


-- ----------------------------------------------------------------------------
-- Auxiliary: Check message ownership
-- Verifies that a message belongs to a folder owned by the authenticated user
-- Parameters: $1=message_id, $2=user_id
-- Returns: message details if user owns the folder containing it
-- ----------------------------------------------------------------------------
SELECT m.id, m.folder_id, m.sender_id, m.recipient_email, m.subject, m.body, m.is_sent, m.created_at
FROM messages m
JOIN folders f ON m.folder_id = f.id
WHERE m.id = $1 AND f.user_id = $2;

-- Example:
-- SELECT m.id, m.folder_id, m.sender_id, m.recipient_email, m.subject, m.body, m.is_sent, m.created_at
-- FROM messages m
-- JOIN folders f ON m.folder_id = f.id
-- WHERE m.id = 1 AND f.user_id = 1;


-- ============================================================================
-- ADDITIONAL USEFUL QUERIES
-- ============================================================================

-- ----------------------------------------------------------------------------
-- Get user statistics
-- Returns: number of folders and messages for a user
-- ----------------------------------------------------------------------------
SELECT 
    u.id,
    u.login,
    u.email,
    u.first_name,
    u.last_name,
    COUNT(DISTINCT f.id) as folder_count,
    COUNT(DISTINCT m.id) as message_count
FROM users u
LEFT JOIN folders f ON u.id = f.user_id
LEFT JOIN messages m ON f.id = m.folder_id
WHERE u.id = $1
GROUP BY u.id, u.login, u.email, u.first_name, u.last_name;


-- ----------------------------------------------------------------------------
-- Get recent messages across all folders for a user
-- Parameters: $1=user_id, $2=limit (e.g., 10)
-- Returns: most recent messages from all user's folders
-- ----------------------------------------------------------------------------
SELECT m.id, m.folder_id, m.sender_id, m.recipient_email, m.subject, m.body, m.is_sent, m.created_at,
       f.name as folder_name,
       u.login as sender_login, u.first_name as sender_first_name, u.last_name as sender_last_name
FROM messages m
JOIN folders f ON m.folder_id = f.id
JOIN users u ON m.sender_id = u.id
WHERE f.user_id = $1
ORDER BY m.created_at DESC
LIMIT $2;


-- ----------------------------------------------------------------------------
-- Search messages by subject or body
-- Parameters: $1=user_id, $2=search_term
-- Returns: messages matching the search term
-- ----------------------------------------------------------------------------
SELECT m.id, m.folder_id, m.sender_id, m.recipient_email, m.subject, m.body, m.is_sent, m.created_at,
       f.name as folder_name
FROM messages m
JOIN folders f ON m.folder_id = f.id
WHERE f.user_id = $1 
  AND (m.subject ILIKE $2 OR m.body ILIKE $2)
ORDER BY m.created_at DESC;


-- ----------------------------------------------------------------------------
-- Get message count per folder for a user
-- Parameters: $1=user_id
-- Returns: folder list with message counts
-- ----------------------------------------------------------------------------
SELECT f.id, f.name, f.created_at, COUNT(m.id) as message_count
FROM folders f
LEFT JOIN messages m ON f.id = m.folder_id
WHERE f.user_id = $1
GROUP BY f.id, f.name, f.created_at
ORDER BY f.created_at ASC;


-- ----------------------------------------------------------------------------
-- Delete old messages (cleanup query)
-- Parameters: $1=days_old (e.g., 90 for messages older than 90 days)
-- Returns: number of deleted messages
-- ----------------------------------------------------------------------------
DELETE FROM messages
WHERE created_at < CURRENT_TIMESTAMP - INTERVAL '1 day' * $1
RETURNING id;


-- ============================================================================
-- PERFORMANCE TESTING QUERIES
-- ============================================================================

-- Test query performance with EXPLAIN ANALYZE
-- Example: Check if index is used for user login search
EXPLAIN ANALYZE
SELECT id, login, email, first_name, last_name, created_at
FROM users
WHERE login = 'alice.smith';

-- Example: Check if index is used for folder listing
EXPLAIN ANALYZE
SELECT id, user_id, name, created_at
FROM folders
WHERE user_id = 1
ORDER BY created_at ASC;

-- Example: Check if index is used for message listing with JOIN
EXPLAIN ANALYZE
SELECT m.id, m.folder_id, m.sender_id, m.recipient_email, m.subject, m.body, m.is_sent, m.created_at,
       u.login as sender_login, u.first_name as sender_first_name, u.last_name as sender_last_name
FROM messages m
JOIN users u ON m.sender_id = u.id
WHERE m.folder_id = 1
ORDER BY m.created_at DESC;

-- ============================================================================
-- END OF QUERIES
-- ============================================================================
