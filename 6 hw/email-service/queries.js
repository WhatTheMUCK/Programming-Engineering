// MongoDB CRUD Operations for Email Service
// Variant 9: Email Service

db = db.getSiblingDB('email_db');

print("=== CREATE OPERATIONS ===");

// Q1: Create User
const newUser = {
  login: "test_user",
  email: "test@example.com",
  firstName: "Test",
  lastName: "User",
  passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8",
  createdAt: new Date(),
  updatedAt: new Date()
};
const userResult = db.users.insertOne(newUser);
print("Q1 Create User: " + userResult.insertedId);

// Q4: Create Folder
const john = db.users.findOne({login: "john_doe"});
const newFolder = {
  userId: john._id,
  name: "Test Folder",
  createdAt: new Date(),
  updatedAt: new Date()
};
const folderResult = db.folders.insertOne(newFolder);
print("Q4 Create Folder: " + folderResult.insertedId);

// Q6: Create Message
const inbox = db.folders.findOne({userId: john._id, name: "Inbox"});
const newMessage = {
  folderId: inbox._id,
  sender: {_id: john._id, login: "john_doe", email: "john@example.com", firstName: "John", lastName: "Doe"},
  recipientEmail: "test@example.com",
  subject: "Test Subject",
  body: "Test body",
  isSent: false,
  createdAt: new Date(),
  updatedAt: new Date()
};
const messageResult = db.messages.insertOne(newMessage);
print("Q6 Create Message: " + messageResult.insertedId);

print("\n=== READ OPERATIONS ===");

// Q2: Find User by Login
const userByLogin = db.users.findOne({login: "john_doe"});
print("Q2 Find User: " + userByLogin.login + " (" + userByLogin.email + ")");

// Q3: Search Users by Name Mask
const usersByName = db.users.find({
  $or: [{firstName: {$regex: "John", $options: "i"}}, {lastName: {$regex: "Doe", $options: "i"}}]
}).toArray();
print("Q3 Search Users: found " + usersByName.length + " users");
usersByName.forEach(function(u) { print("  - " + u.login + ": " + u.firstName + " " + u.lastName); });

// Q5: List Folders
const folders = db.folders.find({userId: john._id}).toArray();
print("Q5 List Folders: " + folders.length + " folders");
folders.forEach(function(f) { print("  - " + f.name); });

// Q7: List Messages in Folder
const messages = db.messages.find({folderId: inbox._id}).sort({createdAt: -1}).limit(3).toArray();
print("Q7 List Messages: " + messages.length + " messages");
messages.forEach(function(m) { print("  - " + m.subject + " from:" + m.sender.login); });

// Q8: Get Message by ID
const msg = db.messages.findOne({_id: messageResult.insertedId});
print("Q8 Get Message: " + msg.subject + " to:" + msg.recipientEmail);

print("\n=== UPDATE OPERATIONS ===");

// Update User
db.users.updateOne({login: "test_user"}, {$set: {email: "updated@example.com"}, $currentDate: {updatedAt: true}});
print("Update User: email changed to " + db.users.findOne({login: "test_user"}).email);

// Update Folder
db.folders.updateOne({_id: folderResult.insertedId}, {$set: {name: "Updated Folder"}});
print("Update Folder: renamed to " + db.folders.findOne({_id: folderResult.insertedId}).name);

// Update Message
db.messages.updateOne({_id: messageResult.insertedId}, {$set: {isSent: true}});
print("Update Message: isSent=" + db.messages.findOne({_id: messageResult.insertedId}).isSent);

// $inc operator
db.users.updateOne({login: "john_doe"}, {$inc: {loginAttempts: 1}});
print("Update $inc: loginAttempts=" + db.users.findOne({login: "john_doe"}).loginAttempts);

print("\n=== DELETE OPERATIONS ===");

const delMsg = db.messages.deleteOne({_id: messageResult.insertedId});
print("Delete Message: " + delMsg.deletedCount + " deleted");

const delFold = db.folders.deleteOne({_id: folderResult.insertedId});
print("Delete Folder: " + delFold.deletedCount + " deleted");

const delUsr = db.users.deleteOne({login: "test_user"});
print("Delete User: " + delUsr.deletedCount + " deleted");

print("\n=== AGGREGATION ===");

print("Messages per folder:");
db.messages.aggregate([
  {$group: {_id: "$folderId", count: {$sum: 1}}},
  {$sort: {count: -1}},
  {$limit: 5},
  {$lookup: {from: "folders", localField: "_id", foreignField: "_id", as: "folder"}},
  {$unwind: "$folder"},
  {$project: {folderName: "$folder.name", count: 1, _id: 0}}
]).forEach(function(doc) { print("  - " + doc.folderName + ": " + doc.count); });

print("\nMessages by sender (top 5):");
db.messages.aggregate([
  {$group: {_id: "$sender.login", count: {$sum: 1}}},
  {$sort: {count: -1}},
  {$limit: 5}
]).forEach(function(doc) { print("  - " + doc._id + ": " + doc.count); });

print("\n=== ADVANCED QUERIES ===");

// $in operator
print("$in operator:");
db.users.find({login: {$in: ["john_doe", "jane_smith", "bob_wilson"]}}).forEach(function(u) { print("  - " + u.login); });

// $gt, $lt
print("$gt operator: " + db.messages.find({createdAt: {$gt: new Date("2024-01-16")}}).count() + " messages after 2024-01-16");

// $exists
print("$exists operator:");
db.users.find({loginAttempts: {$exists: true}}).forEach(function(u) { print("  - " + u.login + " has loginAttempts"); });

print("\nDone!");
