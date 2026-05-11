// MongoDB Test Data for Email Service
// Variant 9: Email Service

db = db.getSiblingDB('email_db');

// Clear existing data
db.users.deleteMany({});
db.folders.deleteMany({});
db.messages.deleteMany({});

// Insert users (12 users)
const userResult = db.users.insertMany([
  { login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe", passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", createdAt: new Date("2024-01-15T10:30:00Z"), updatedAt: new Date("2024-01-15T10:30:00Z") },
  { login: "jane_smith", email: "jane@example.com", firstName: "Jane", lastName: "Smith", passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", createdAt: new Date("2024-01-16T11:45:00Z"), updatedAt: new Date("2024-01-16T11:45:00Z") },
  { login: "bob_wilson", email: "bob@example.com", firstName: "Bob", lastName: "Wilson", passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", createdAt: new Date("2024-01-17T09:15:00Z"), updatedAt: new Date("2024-01-17T09:15:00Z") },
  { login: "alice_johnson", email: "alice@example.com", firstName: "Alice", lastName: "Johnson", passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", createdAt: new Date("2024-01-18T14:20:00Z"), updatedAt: new Date("2024-01-18T14:20:00Z") },
  { login: "charlie_brown", email: "charlie@example.com", firstName: "Charlie", lastName: "Brown", passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", createdAt: new Date("2024-01-19T16:30:00Z"), updatedAt: new Date("2024-01-19T16:30:00Z") },
  { login: "diana_prince", email: "diana@example.com", firstName: "Diana", lastName: "Prince", passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", createdAt: new Date("2024-01-20T08:00:00Z"), updatedAt: new Date("2024-01-20T08:00:00Z") },
  { login: "evan_wright", email: "evan@example.com", firstName: "Evan", lastName: "Wright", passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", createdAt: new Date("2024-01-21T12:45:00Z"), updatedAt: new Date("2024-01-21T12:45:00Z") },
  { login: "fiona_green", email: "fiona@example.com", firstName: "Fiona", lastName: "Green", passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", createdAt: new Date("2024-01-22T15:30:00Z"), updatedAt: new Date("2024-01-22T15:30:00Z") },
  { login: "george_hall", email: "george@example.com", firstName: "George", lastName: "Hall", passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", createdAt: new Date("2024-01-23T10:15:00Z"), updatedAt: new Date("2024-01-23T10:15:00Z") },
  { login: "hannah_white", email: "hannah@example.com", firstName: "Hannah", lastName: "White", passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", createdAt: new Date("2024-01-24T13:00:00Z"), updatedAt: new Date("2024-01-24T13:00:00Z") },
  { login: "ian_miller", email: "ian@example.com", firstName: "Ian", lastName: "Miller", passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", createdAt: new Date("2024-01-25T09:30:00Z"), updatedAt: new Date("2024-01-25T09:30:00Z") },
  { login: "julia_davis", email: "julia@example.com", firstName: "Julia", lastName: "Davis", passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", createdAt: new Date("2024-01-26T11:20:00Z"), updatedAt: new Date("2024-01-26T11:20:00Z") }
]);

// Get inserted user ObjectIds
const userIds = Object.keys(userResult.insertedIds).map(k => userResult.insertedIds[k]);

// Insert folders (3 per user = 36 folders)
const folders = [];
for (let i = 0; i < 12; i++) {
  folders.push(
    { userId: userIds[i], name: "Inbox", createdAt: new Date(), updatedAt: new Date() },
    { userId: userIds[i], name: "Sent", createdAt: new Date(), updatedAt: new Date() },
    { userId: userIds[i], name: "Drafts", createdAt: new Date(), updatedAt: new Date() }
  );
}
const folderResult = db.folders.insertMany(folders);
const folderIds = Object.keys(folderResult.insertedIds).map(k => folderResult.insertedIds[k]);

// Insert messages (50 messages)
const messages = [
  // john_doe's Inbox (folder 0)
  { folderId: folderIds[0], sender: { _id: userIds[1], login: "jane_smith", email: "jane@example.com", firstName: "Jane", lastName: "Smith" }, recipientEmail: "john@example.com", subject: "Meeting tomorrow", body: "Let's meet at 10 AM", isSent: false, createdAt: new Date("2024-01-15T11:00:00Z"), updatedAt: new Date("2024-01-15T11:00:00Z") },
  { folderId: folderIds[0], sender: { _id: userIds[2], login: "bob_wilson", email: "bob@example.com", firstName: "Bob", lastName: "Wilson" }, recipientEmail: "john@example.com", subject: "Project update", body: "Phase 1 completed", isSent: false, createdAt: new Date("2024-01-15T12:30:00Z"), updatedAt: new Date("2024-01-15T12:30:00Z") },
  { folderId: folderIds[0], sender: { _id: userIds[3], login: "alice_johnson", email: "alice@example.com", firstName: "Alice", lastName: "Johnson" }, recipientEmail: "john@example.com", subject: "Question about report", body: "Clarify section 3", isSent: false, createdAt: new Date("2024-01-15T14:00:00Z"), updatedAt: new Date("2024-01-15T14:00:00Z") },
  { folderId: folderIds[0], sender: { _id: userIds[4], login: "charlie_brown", email: "charlie@example.com", firstName: "Charlie", lastName: "Brown" }, recipientEmail: "john@example.com", subject: "Lunch?", body: "Want to grab lunch?", isSent: false, createdAt: new Date("2024-01-15T15:30:00Z"), updatedAt: new Date("2024-01-15T15:30:00Z") },
  { folderId: folderIds[0], sender: { _id: userIds[5], login: "diana_prince", email: "diana@example.com", firstName: "Diana", lastName: "Prince" }, recipientEmail: "john@example.com", subject: "Conference invite", body: "You are invited", isSent: false, createdAt: new Date("2024-01-15T16:45:00Z"), updatedAt: new Date("2024-01-15T16:45:00Z") },
  
  // john_doe's Sent (folder 1)
  { folderId: folderIds[1], sender: { _id: userIds[0], login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe" }, recipientEmail: "jane@example.com", subject: "Re: Meeting", body: "10 AM works for me", isSent: true, createdAt: new Date("2024-01-15T11:15:00Z"), updatedAt: new Date("2024-01-15T11:15:00Z") },
  { folderId: folderIds[1], sender: { _id: userIds[0], login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe" }, recipientEmail: "bob@example.com", subject: "Re: Project update", body: "Good job!", isSent: true, createdAt: new Date("2024-01-15T12:45:00Z"), updatedAt: new Date("2024-01-15T12:45:00Z") },
  { folderId: folderIds[1], sender: { _id: userIds[0], login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe" }, recipientEmail: "alice@example.com", subject: "Re: Question", body: "Section 3 clarified", isSent: true, createdAt: new Date("2024-01-15T14:15:00Z"), updatedAt: new Date("2024-01-15T14:15:00Z") },
  
  // jane_smith's Inbox (folder 3)
  { folderId: folderIds[3], sender: { _id: userIds[0], login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe" }, recipientEmail: "jane@example.com", subject: "Hello", body: "Hi Jane!", isSent: false, createdAt: new Date("2024-01-16T12:00:00Z"), updatedAt: new Date("2024-01-16T12:00:00Z") },
  { folderId: folderIds[3], sender: { _id: userIds[2], login: "bob_wilson", email: "bob@example.com", firstName: "Bob", lastName: "Wilson" }, recipientEmail: "jane@example.com", subject: "Report", body: "Here's the report", isSent: false, createdAt: new Date("2024-01-16T13:00:00Z"), updatedAt: new Date("2024-01-16T13:00:00Z") },
  
  // jane_smith's Sent (folder 4)
  { folderId: folderIds[4], sender: { _id: userIds[1], login: "jane_smith", email: "jane@example.com", firstName: "Jane", lastName: "Smith" }, recipientEmail: "john@example.com", subject: "Re: Hello", body: "Hi John!", isSent: true, createdAt: new Date("2024-01-16T12:10:00Z"), updatedAt: new Date("2024-01-16T12:10:00Z") },
  
  // bob_wilson folders
  { folderId: folderIds[6], sender: { _id: userIds[1], login: "jane_smith", email: "jane@example.com", firstName: "Jane", lastName: "Smith" }, recipientEmail: "bob@example.com", subject: "Hey Bob", body: "How are you?", isSent: false, createdAt: new Date("2024-01-17T10:00:00Z"), updatedAt: new Date("2024-01-17T10:00:00Z") },
  { folderId: folderIds[7], sender: { _id: userIds[2], login: "bob_wilson", email: "bob@example.com", firstName: "Bob", lastName: "Wilson" }, recipientEmail: "alice@example.com", subject: "Hi", body: "Hello Alice", isSent: true, createdAt: new Date("2024-01-17T10:30:00Z"), updatedAt: new Date("2024-01-17T10:30:00Z") },
  
  // alice_johnson folders
  { folderId: folderIds[9], sender: { _id: userIds[0], login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe" }, recipientEmail: "alice@example.com", subject: "Welcome", body: "Welcome to the team!", isSent: false, createdAt: new Date("2024-01-18T15:00:00Z"), updatedAt: new Date("2024-01-18T15:00:00Z") },
  { folderId: folderIds[9], sender: { _id: userIds[1], login: "jane_smith", email: "jane@example.com", firstName: "Jane", lastName: "Smith" }, recipientEmail: "alice@example.com", subject: "Hi Alice", body: "Nice to meet you", isSent: false, createdAt: new Date("2024-01-18T15:30:00Z"), updatedAt: new Date("2024-01-18T15:30:00Z") },
  { folderId: folderIds[10], sender: { _id: userIds[3], login: "alice_johnson", email: "alice@example.com", firstName: "Alice", lastName: "Johnson" }, recipientEmail: "john@example.com", subject: "Thanks", body: "Thank you!", isSent: true, createdAt: new Date("2024-01-18T15:15:00Z"), updatedAt: new Date("2024-01-18T15:15:00Z") },
  
  // charlie_brown folders
  { folderId: folderIds[12], sender: { _id: userIds[5], login: "diana_prince", email: "diana@example.com", firstName: "Diana", lastName: "Prince" }, recipientEmail: "charlie@example.com", subject: "Hello Charlie", body: "Long time no see", isSent: false, createdAt: new Date("2024-01-19T17:00:00Z"), updatedAt: new Date("2024-01-19T17:00:00Z") },
  { folderId: folderIds[13], sender: { _id: userIds[4], login: "charlie_brown", email: "charlie@example.com", firstName: "Charlie", lastName: "Brown" }, recipientEmail: "diana@example.com", subject: "Re: Hello", body: "Hi Diana!", isSent: true, createdAt: new Date("2024-01-19T17:15:00Z"), updatedAt: new Date("2024-01-19T17:15:00Z") },
  
  // diana_prince folders
  { folderId: folderIds[15], sender: { _id: userIds[6], login: "evan_wright", email: "evan@example.com", firstName: "Evan", lastName: "Wright" }, recipientEmail: "diana@example.com", subject: "Meeting request", body: "Can we meet?", isSent: false, createdAt: new Date("2024-01-20T09:00:00Z"), updatedAt: new Date("2024-01-20T09:00:00Z") },
  { folderId: folderIds[16], sender: { _id: userIds[5], login: "diana_prince", email: "diana@example.com", firstName: "Diana", lastName: "Prince" }, recipientEmail: "evan@example.com", subject: "Re: Meeting", body: "Sure, when?", isSent: true, createdAt: new Date("2024-01-20T09:15:00Z"), updatedAt: new Date("2024-01-20T09:15:00Z") },
  
  // evan_wright folders
  { folderId: folderIds[18], sender: { _id: userIds[7], login: "fiona_green", email: "fiona@example.com", firstName: "Fiona", lastName: "Green" }, recipientEmail: "evan@example.com", subject: "Hi Evan", body: "How's it going?", isSent: false, createdAt: new Date("2024-01-21T13:00:00Z"), updatedAt: new Date("2024-01-21T13:00:00Z") },
  { folderId: folderIds[19], sender: { _id: userIds[6], login: "evan_wright", email: "evan@example.com", firstName: "Evan", lastName: "Wright" }, recipientEmail: "fiona@example.com", subject: "Re: Hi", body: "Good, thanks!", isSent: true, createdAt: new Date("2024-01-21T13:15:00Z"), updatedAt: new Date("2024-01-21T13:15:00Z") },
  
  // fiona_green folders
  { folderId: folderIds[21], sender: { _id: userIds[8], login: "george_hall", email: "george@example.com", firstName: "George", lastName: "Hall" }, recipientEmail: "fiona@example.com", subject: "Project", body: "Let's discuss", isSent: false, createdAt: new Date("2024-01-22T16:00:00Z"), updatedAt: new Date("2024-01-22T16:00:00Z") },
  { folderId: folderIds[22], sender: { _id: userIds[7], login: "fiona_green", email: "fiona@example.com", firstName: "Fiona", lastName: "Green" }, recipientEmail: "george@example.com", subject: "Re: Project", body: "OK, let's meet", isSent: true, createdAt: new Date("2024-01-22T16:30:00Z"), updatedAt: new Date("2024-01-22T16:30:00Z") },
  
  // george_hall folders
  { folderId: folderIds[24], sender: { _id: userIds[9], login: "hannah_white", email: "hannah@example.com", firstName: "Hannah", lastName: "White" }, recipientEmail: "george@example.com", subject: "Hello George", body: "Nice to meet you", isSent: false, createdAt: new Date("2024-01-23T11:00:00Z"), updatedAt: new Date("2024-01-23T11:00:00Z") },
  { folderId: folderIds[25], sender: { _id: userIds[8], login: "george_hall", email: "george@example.com", firstName: "George", lastName: "Hall" }, recipientEmail: "hannah@example.com", subject: "Re: Hello", body: "Hi Hannah!", isSent: true, createdAt: new Date("2024-01-23T11:15:00Z"), updatedAt: new Date("2024-01-23T11:15:00Z") },
  
  // hannah_white folders
  { folderId: folderIds[27], sender: { _id: userIds[10], login: "ian_miller", email: "ian@example.com", firstName: "Ian", lastName: "Miller" }, recipientEmail: "hannah@example.com", subject: "Hey", body: "What's up?", isSent: false, createdAt: new Date("2024-01-24T14:00:00Z"), updatedAt: new Date("2024-01-24T14:00:00Z") },
  { folderId: folderIds[28], sender: { _id: userIds[9], login: "hannah_white", email: "hannah@example.com", firstName: "Hannah", lastName: "White" }, recipientEmail: "ian@example.com", subject: "Re: Hey", body: "Not much", isSent: true, createdAt: new Date("2024-01-24T14:15:00Z"), updatedAt: new Date("2024-01-24T14:15:00Z") },
  
  // ian_miller folders
  { folderId: folderIds[30], sender: { _id: userIds[11], login: "julia_davis", email: "julia@example.com", firstName: "Julia", lastName: "Davis" }, recipientEmail: "ian@example.com", subject: "Hi Ian", body: "How are you?", isSent: false, createdAt: new Date("2024-01-25T10:00:00Z"), updatedAt: new Date("2024-01-25T10:00:00Z") },
  { folderId: folderIds[31], sender: { _id: userIds[10], login: "ian_miller", email: "ian@example.com", firstName: "Ian", lastName: "Miller" }, recipientEmail: "julia@example.com", subject: "Re: Hi", body: "I'm good!", isSent: true, createdAt: new Date("2024-01-25T10:15:00Z"), updatedAt: new Date("2024-01-25T10:15:00Z") },
  
  // julia_davis folders
  { folderId: folderIds[33], sender: { _id: userIds[0], login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe" }, recipientEmail: "julia@example.com", subject: "Welcome Julia", body: "Welcome to the team!", isSent: false, createdAt: new Date("2024-01-26T12:00:00Z"), updatedAt: new Date("2024-01-26T12:00:00Z") },
  { folderId: folderIds[34], sender: { _id: userIds[1], login: "jane_smith", email: "jane@example.com", firstName: "Jane", lastName: "Smith" }, recipientEmail: "julia@example.com", subject: "Hi Julia", body: "Nice to meet you!", isSent: false, createdAt: new Date("2024-01-26T12:30:00Z"), updatedAt: new Date("2024-01-26T12:30:00Z") },
  { folderId: folderIds[35], sender: { _id: userIds[11], login: "julia_davis", email: "julia@example.com", firstName: "Julia", lastName: "Davis" }, recipientEmail: "john@example.com", subject: "Re: Welcome", body: "Thank you!", isSent: true, createdAt: new Date("2024-01-26T12:15:00Z"), updatedAt: new Date("2024-01-26T12:15:00Z") },
  
  // Additional messages to reach 50
  { folderId: folderIds[0], sender: { _id: userIds[6], login: "evan_wright", email: "evan@example.com", firstName: "Evan", lastName: "Wright" }, recipientEmail: "john@example.com", subject: "Quick question", body: "Do you have a minute?", isSent: false, createdAt: new Date("2024-01-15T17:00:00Z"), updatedAt: new Date("2024-01-15T17:00:00Z") },
  { folderId: folderIds[0], sender: { _id: userIds[7], login: "fiona_green", email: "fiona@example.com", firstName: "Fiona", lastName: "Green" }, recipientEmail: "john@example.com", subject: "Follow up", body: "About yesterday", isSent: false, createdAt: new Date("2024-01-15T18:00:00Z"), updatedAt: new Date("2024-01-15T18:00:00Z") },
  { folderId: folderIds[0], sender: { _id: userIds[8], login: "george_hall", email: "george@example.com", firstName: "George", lastName: "Hall" }, recipientEmail: "john@example.com", subject: "Update", body: "Latest updates", isSent: false, createdAt: new Date("2024-01-15T19:00:00Z"), updatedAt: new Date("2024-01-15T19:00:00Z") },
  { folderId: folderIds[0], sender: { _id: userIds[9], login: "hannah_white", email: "hannah@example.com", firstName: "Hannah", lastName: "White" }, recipientEmail: "john@example.com", subject: "Request", body: "Can you help?", isSent: false, createdAt: new Date("2024-01-15T20:00:00Z"), updatedAt: new Date("2024-01-15T20:00:00Z") },
  { folderId: folderIds[0], sender: { _id: userIds[10], login: "ian_miller", email: "ian@example.com", firstName: "Ian", lastName: "Miller" }, recipientEmail: "john@example.com", subject: "Info", body: "Important info", isSent: false, createdAt: new Date("2024-01-15T21:00:00Z"), updatedAt: new Date("2024-01-15T21:00:00Z") },
  { folderId: folderIds[0], sender: { _id: userIds[11], login: "julia_davis", email: "julia@example.com", firstName: "Julia", lastName: "Davis" }, recipientEmail: "john@example.com", subject: "Hello", body: "Just saying hi", isSent: false, createdAt: new Date("2024-01-15T22:00:00Z"), updatedAt: new Date("2024-01-15T22:00:00Z") },
  { folderId: folderIds[1], sender: { _id: userIds[0], login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe" }, recipientEmail: "charlie@example.com", subject: "Re: Lunch", body: "Sure, let's go", isSent: true, createdAt: new Date("2024-01-15T15:45:00Z"), updatedAt: new Date("2024-01-15T15:45:00Z") },
  { folderId: folderIds[1], sender: { _id: userIds[0], login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe" }, recipientEmail: "diana@example.com", subject: "Re: Conference", body: "I'll be there", isSent: true, createdAt: new Date("2024-01-15T17:00:00Z"), updatedAt: new Date("2024-01-15T17:00:00Z") },
  { folderId: folderIds[1], sender: { _id: userIds[0], login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe" }, recipientEmail: "evan@example.com", subject: "Re: Question", body: "Yes, I do", isSent: true, createdAt: new Date("2024-01-15T17:15:00Z"), updatedAt: new Date("2024-01-15T17:15:00Z") },
  { folderId: folderIds[1], sender: { _id: userIds[0], login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe" }, recipientEmail: "fiona@example.com", subject: "Re: Follow up", body: "Got it", isSent: true, createdAt: new Date("2024-01-15T18:15:00Z"), updatedAt: new Date("2024-01-15T18:15:00Z") },
  { folderId: folderIds[1], sender: { _id: userIds[0], login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe" }, recipientEmail: "george@example.com", subject: "Re: Update", body: "Thanks for update", isSent: true, createdAt: new Date("2024-01-15T19:15:00Z"), updatedAt: new Date("2024-01-15T19:15:00Z") },
  { folderId: folderIds[1], sender: { _id: userIds[0], login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe" }, recipientEmail: "hannah@example.com", subject: "Re: Request", body: "Of course", isSent: true, createdAt: new Date("2024-01-15T20:15:00Z"), updatedAt: new Date("2024-01-15T20:15:00Z") },
  { folderId: folderIds[1], sender: { _id: userIds[0], login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe" }, recipientEmail: "ian@example.com", subject: "Re: Info", body: "Very helpful", isSent: true, createdAt: new Date("2024-01-15T21:15:00Z"), updatedAt: new Date("2024-01-15T21:15:00Z") },
  { folderId: folderIds[1], sender: { _id: userIds[0], login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe" }, recipientEmail: "julia@example.com", subject: "Re: Hello", body: "Hi Julia!", isSent: true, createdAt: new Date("2024-01-15T22:15:00Z"), updatedAt: new Date("2024-01-15T22:15:00Z") }
];

db.messages.insertMany(messages);

// Print summary
print("=== Data Inserted ===");
print("Users: " + db.users.countDocuments({}));
print("Folders: " + db.folders.countDocuments({}));
print("Messages: " + db.messages.countDocuments({}));
