-- ============================================================================
-- Email Service Test Data
-- Variant 9: Электронная почта (Email Service)
-- ============================================================================

-- ============================================================================
-- Insert test users (10+ users)
-- Password for all users: "securePassword123" (hashed with SHA-256)
-- Hash: 4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d
-- ============================================================================

INSERT INTO users (login, email, first_name, last_name, password_hash) VALUES
('alice.smith', 'alice.smith@example.com', 'Alice', 'Smith', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d'),
('bob.jones', 'bob.jones@example.com', 'Bob', 'Jones', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d'),
('charlie.brown', 'charlie.brown@example.com', 'Charlie', 'Brown', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d'),
('diana.prince', 'diana.prince@example.com', 'Diana', 'Prince', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d'),
('edward.norton', 'edward.norton@example.com', 'Edward', 'Norton', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d'),
('fiona.apple', 'fiona.apple@example.com', 'Fiona', 'Apple', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d'),
('george.martin', 'george.martin@example.com', 'George', 'Martin', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d'),
('helen.mirren', 'helen.mirren@example.com', 'Helen', 'Mirren', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d'),
('ivan.petrov', 'ivan.petrov@example.com', 'Ivan', 'Petrov', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d'),
('julia.roberts', 'julia.roberts@example.com', 'Julia', 'Roberts', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d'),
('kevin.spacey', 'kevin.spacey@example.com', 'Kevin', 'Spacey', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d'),
('laura.palmer', 'laura.palmer@example.com', 'Laura', 'Palmer', '4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d')
ON CONFLICT (login) DO NOTHING;

-- ============================================================================
-- Insert test folders (10+ folders across different users)
-- Standard folders: Inbox, Sent, Drafts, Spam, Trash, Archive
-- NOTE: john_doe (user_id=1) already has folders 1-3 created in init-db.sql
-- ============================================================================

INSERT INTO folders (user_id, name) VALUES
-- Alice's folders (user_id = 2, folder_id = 4-8)
(2, 'Inbox'),
(2, 'Sent'),
(2, 'Drafts'),
(2, 'Spam'),
(2, 'Trash'),

-- Bob's folders (user_id = 3, folder_id = 9-12)
(3, 'Inbox'),
(3, 'Sent'),
(3, 'Work'),
(3, 'Personal'),

-- Charlie's folders (user_id = 4, folder_id = 13-15)
(4, 'Inbox'),
(4, 'Sent'),
(4, 'Archive'),

-- Diana's folders (user_id = 5, folder_id = 16-18)
(5, 'Inbox'),
(5, 'Sent'),
(5, 'Important'),

-- Edward's folders (user_id = 6, folder_id = 19-20)
(6, 'Inbox'),
(6, 'Sent'),

-- Fiona's folders (user_id = 7, folder_id = 21-23)
(7, 'Inbox'),
(7, 'Sent'),
(7, 'Music'),

-- George's folders (user_id = 8, folder_id = 24-25)
(8, 'Inbox'),
(8, 'Sent'),

-- Helen's folders (user_id = 9, folder_id = 26-27)
(9, 'Inbox'),
(9, 'Sent'),

-- Ivan's folders (user_id = 10, folder_id = 28-29)
(10, 'Inbox'),
(10, 'Sent'),

-- Julia's folders (user_id = 11, folder_id = 30-31)
(11, 'Inbox'),
(11, 'Sent'),

-- Kevin's folders (user_id = 12, folder_id = 32-33)
(12, 'Inbox'),
(12, 'Sent'),

-- Laura's folders (user_id = 13, folder_id = 34-35)
(13, 'Inbox'),
(13, 'Sent')
ON CONFLICT (user_id, name) DO NOTHING;

-- ============================================================================
-- Insert test messages (15+ messages across different folders)
-- NOTE: folder_id references must match the folders created above
-- john_doe (user_id=1): folders 1-3 (Inbox, Sent, Drafts) - created in init-db.sql
-- alice.smith (user_id=2): folders 4-8 (Inbox, Sent, Drafts, Spam, Trash)
-- bob.jones (user_id=3): folders 9-12 (Inbox, Sent, Work, Personal)
-- charlie.brown (user_id=4): folders 13-15 (Inbox, Sent, Archive)
-- diana.prince (user_id=5): folders 16-18 (Inbox, Sent, Important)
-- edward.norton (user_id=6): folders 19-20 (Inbox, Sent)
-- fiona.apple (user_id=7): folders 21-23 (Inbox, Sent, Music)
-- george.martin (user_id=8): folders 24-25 (Inbox, Sent)
-- helen.mirren (user_id=9): folders 26-27 (Inbox, Sent)
-- ivan.petrov (user_id=10): folders 28-29 (Inbox, Sent)
-- julia.roberts (user_id=11): folders 30-31 (Inbox, Sent)
-- kevin.spacey (user_id=12): folders 32-33 (Inbox, Sent)
-- laura.palmer (user_id=13): folders 34-35 (Inbox, Sent)
-- ============================================================================

-- Insert messages using subqueries to get correct folder IDs
INSERT INTO messages (folder_id, sender_id, recipient_email, subject, body, is_sent, created_at) VALUES
-- Alice's messages (in her Inbox)
((SELECT id FROM folders WHERE user_id = 2 AND name = 'Inbox'), 2, 'alice.smith@example.com', 'Welcome to our platform!', 'Hi Alice, welcome to our email service. We hope you enjoy using it!', true, '2024-01-15 10:30:00'),
((SELECT id FROM folders WHERE user_id = 2 AND name = 'Inbox'), 3, 'alice.smith@example.com', 'Meeting reminder', 'Don''t forget about our meeting tomorrow at 2 PM.', true, '2024-01-16 14:20:00'),
((SELECT id FROM folders WHERE user_id = 2 AND name = 'Inbox'), 4, 'alice.smith@example.com', 'Project update', 'The project is progressing well. Here are the latest updates...', true, '2024-01-17 09:15:00'),
((SELECT id FROM folders WHERE user_id = 2 AND name = 'Drafts'), 1, 'bob.jones@example.com', 'Draft: Proposal', 'This is a draft of the proposal I''m working on...', false, '2024-01-18 16:45:00'),

-- Bob's messages (in his Inbox)
((SELECT id FROM folders WHERE user_id = 3 AND name = 'Inbox'), 1, 'bob.jones@example.com', 'Re: Welcome', 'Thanks for the warm welcome!', true, '2024-01-15 11:00:00'),
((SELECT id FROM folders WHERE user_id = 3 AND name = 'Inbox'), 3, 'bob.jones@example.com', 'Lunch plans', 'Hey Bob, want to grab lunch tomorrow?', true, '2024-01-16 12:30:00'),
((SELECT id FROM folders WHERE user_id = 3 AND name = 'Work'), 2, 'manager@company.com', 'Weekly report', 'Here is my weekly report for the team...', true, '2024-01-19 17:00:00'),

-- Charlie's messages (in his Inbox)
((SELECT id FROM folders WHERE user_id = 4 AND name = 'Inbox'), 4, 'charlie.brown@example.com', 'Question about the project', 'Hi Charlie, I have a question about the timeline...', true, '2024-01-17 10:00:00'),
((SELECT id FROM folders WHERE user_id = 4 AND name = 'Inbox'), 5, 'charlie.brown@example.com', 'Code review request', 'Could you please review my pull request?', true, '2024-01-18 15:30:00'),
((SELECT id FROM folders WHERE user_id = 4 AND name = 'Sent'), 3, 'diana.prince@example.com', 'Thanks!', 'Thank you for your help with the project.', true, '2024-01-19 11:00:00'),

-- Diana's messages (in her Inbox)
((SELECT id FROM folders WHERE user_id = 5 AND name = 'Inbox'), 6, 'diana.prince@example.com', 'Conference invitation', 'You are invited to speak at our annual conference...', true, '2024-01-20 09:00:00'),
((SELECT id FROM folders WHERE user_id = 5 AND name = 'Inbox'), 7, 'diana.prince@example.com', 'Newsletter: January 2024', 'Here is our monthly newsletter with all the updates...', true, '2024-01-21 08:00:00'),
((SELECT id FROM folders WHERE user_id = 5 AND name = 'Important'), 4, 'urgent@company.com', 'URGENT: Security update', 'Please update your password immediately due to security concerns.', true, '2024-01-22 07:30:00'),

-- Edward's messages (in his Inbox)
((SELECT id FROM folders WHERE user_id = 6 AND name = 'Inbox'), 8, 'edward.norton@example.com', 'Job opportunity', 'We have an exciting job opportunity that might interest you...', true, '2024-01-20 14:00:00'),
((SELECT id FROM folders WHERE user_id = 6 AND name = 'Inbox'), 9, 'edward.norton@example.com', 'Invoice #12345', 'Please find attached the invoice for services rendered...', true, '2024-01-21 10:30:00'),

-- Fiona's messages (in her Inbox)
((SELECT id FROM folders WHERE user_id = 7 AND name = 'Inbox'), 10, 'fiona.apple@example.com', 'New album release', 'Check out my new album dropping next month!', true, '2024-01-22 12:00:00'),
((SELECT id FROM folders WHERE user_id = 7 AND name = 'Music'), 6, 'studio@music.com', 'Studio booking confirmation', 'Your studio session is confirmed for next week.', true, '2024-01-23 09:00:00'),

-- George's messages (in his Inbox)
((SELECT id FROM folders WHERE user_id = 8 AND name = 'Inbox'), 11, 'george.martin@example.com', 'Book recommendation', 'I think you would enjoy this new fantasy novel...', true, '2024-01-23 15:00:00'),

-- Helen's messages (in her Inbox)
((SELECT id FROM folders WHERE user_id = 9 AND name = 'Inbox'), 12, 'helen.mirren@example.com', 'Theater tickets', 'Your tickets for the show are ready for pickup.', true, '2024-01-24 11:00:00'),

-- Ivan's messages (in his Inbox)
((SELECT id FROM folders WHERE user_id = 10 AND name = 'Inbox'), 1, 'ivan.petrov@example.com', 'Привет!', 'Как дела? Давно не виделись!', true, '2024-01-24 16:00:00'),

-- Julia's messages (in her Inbox)
((SELECT id FROM folders WHERE user_id = 11 AND name = 'Inbox'), 2, 'julia.roberts@example.com', 'Movie premiere invitation', 'You are invited to the premiere of our new film...', true, '2024-01-25 10:00:00')
ON CONFLICT DO NOTHING;

-- ============================================================================
-- Verify data insertion
-- ============================================================================

-- Count records in each table
DO $$
DECLARE
    user_count INTEGER;
    folder_count INTEGER;
    message_count INTEGER;
BEGIN
    SELECT COUNT(*) INTO user_count FROM users;
    SELECT COUNT(*) INTO folder_count FROM folders;
    SELECT COUNT(*) INTO message_count FROM messages;
    
    RAISE NOTICE 'Data insertion complete:';
    RAISE NOTICE '  Users: %', user_count;
    RAISE NOTICE '  Folders: %', folder_count;
    RAISE NOTICE '  Messages: %', message_count;
END $$;

-- ============================================================================
-- END OF TEST DATA
-- ============================================================================
