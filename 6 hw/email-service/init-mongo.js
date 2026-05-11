// ============================================================================
// MongoDB Initialization Script for Email Service
// Домашнее задание 4: Миграция с PostgreSQL на MongoDB
// ============================================================================

const dbName = process.env.MONGO_INITDB_DATABASE || 'email_db';

// Переключиться на целевую БД
 db = db.getSiblingDB(dbName);

print("=== MongoDB Initialization for Email Service ===");
print("Database: " + dbName);
print("Date: " + new Date().toISOString());

// Create user for application authentication (if not exists)
print("\n--- Creating application user ---");
try {
  db.createUser({
    user: "email_user",
    pwd: "email_pass",
    roles: [
      { role: "readWrite", db: dbName }
    ]
  });
  print("✓ Created user: email_user");
} catch (e) {
  // Error code 13 = unauthorized (user already exists), 51003 = user already exists
  if (e.code === 13 || e.code === 51003) {
    print("✓ User email_user already exists");
  } else {
    throw e;
  }
}

// ============================================================================
// Создание коллекций с валидацией
// ============================================================================

print("\n--- Creating collections with validation ---");

// Drop existing collections to recreate with correct schema
try { db.users.drop(); } catch(e) {}
try { db.folders.drop(); } catch(e) {}
try { db.messages.drop(); } catch(e) {}

// Создание коллекции users с валидацией
db.createCollection('users', {
  validator: {
    $jsonSchema: {
      bsonType: 'object',
      required: ['login', 'email', 'firstName', 'lastName', 'passwordHash'],
      properties: {
        login: {
          bsonType: 'string',
          minLength: 3,
          maxLength: 255,
          description: 'User login (unique)'
        },
        email: {
          bsonType: 'string',
          pattern: '^[^@]+@[^@]+$',
          description: 'User email (unique)'
        },
        firstName: {
          bsonType: 'string',
          minLength: 1,
          maxLength: 255,
          description: 'User first name'
        },
        lastName: {
          bsonType: 'string',
          minLength: 1,
          maxLength: 255,
          description: 'User last name'
        },
        passwordHash: {
          bsonType: 'string',
          minLength: 64,
          description: 'SHA-256 hash of password'
        },
        createdAt: {
          bsonType: 'date',
          description: 'Creation timestamp'
        },
        updatedAt: {
          bsonType: 'date',
          description: 'Last update timestamp'
        }
      }
    }
  }
});
print("✓ Created collection: users");

// Создание коллекции folders с валидацией
db.createCollection('folders', {
  validator: {
    $jsonSchema: {
      bsonType: 'object',
      required: ['userId', 'name'],
      properties: {
        userId: {
          bsonType: 'objectId',
          description: 'Reference to users._id (MongoDB ObjectId)'
        },
        name: {
          bsonType: 'string',
          minLength: 1,
          maxLength: 255,
          description: 'Folder name'
        },
        createdAt: {
          bsonType: 'date',
          description: 'Creation timestamp'
        },
        updatedAt: {
          bsonType: 'date',
          description: 'Last update timestamp'
        }
      }
    }
  }
});
print("✓ Created collection: folders");

// Создание коллекции messages с валидацией
db.createCollection('messages', {
  validator: {
    $jsonSchema: {
      bsonType: 'object',
      required: ['folderId', 'sender', 'recipientEmail', 'subject', 'body'],
      properties: {
        folderId: {
          bsonType: 'objectId',
          description: 'Reference to folders._id (MongoDB ObjectId)'
        },
        sender: {
          bsonType: 'object',
          required: ['_id', 'login', 'email'],
          properties: {
            _id: {
              bsonType: 'objectId',
              description: 'MongoDB sender document _id'
            },
            login: {
              bsonType: 'string',
              description: 'Sender login'
            },
            email: {
              bsonType: 'string',
              description: 'Sender email'
            },
            firstName: {
              bsonType: 'string',
              description: 'Sender first name (optional)'
            },
            lastName: {
              bsonType: 'string',
              description: 'Sender last name (optional)'
            }
          }
        },
        recipientEmail: {
          bsonType: 'string',
          pattern: '^[^@]+@[^@]+$',
          description: 'Recipient email address'
        },
        subject: {
          bsonType: 'string',
          minLength: 1,
          maxLength: 500,
          description: 'Message subject'
        },
        body: {
          bsonType: 'string',
          minLength: 1,
          description: 'Message body'
        },
        isSent: {
          bsonType: 'bool',
          description: 'Whether message has been sent'
        },
        createdAt: {
          bsonType: 'date',
          description: 'Creation timestamp'
        },
        updatedAt: {
          bsonType: 'date',
          description: 'Last update timestamp'
        }
      }
    }
  }
});
print("✓ Created collection: messages");

// ============================================================================
// Создание индексов
// ============================================================================

print("\n--- Creating indexes ---");

// Индексы для users
db.users.createIndex({ login: 1 }, { unique: true });
print("✓ Created index: users.login (unique)");

db.users.createIndex({ email: 1 }, { unique: true });
print("✓ Created index: users.email (unique)");

db.users.createIndex({ firstName: 1, lastName: 1 });
print("✓ Created index: users.firstName, users.lastName");

db.users.createIndex({ createdAt: -1 });
print("✓ Created index: users.createdAt");

// Индексы для folders
db.folders.createIndex({ userId: 1, name: 1 }, { unique: true });
print("✓ Created index: folders.userId, folders.name (unique)");

db.folders.createIndex({ userId: 1 });
print("✓ Created index: folders.userId");

db.folders.createIndex({ createdAt: -1 });
print("✓ Created index: folders.createdAt");

// Индексы для messages
db.messages.createIndex({ folderId: 1, createdAt: -1 });
print("✓ Created index: messages.folderId, messages.createdAt");

db.messages.createIndex({ folderId: 1 });
print("✓ Created index: messages.folderId");

db.messages.createIndex({ "sender._id": 1 });
print("✓ Created index: messages.sender._id");

db.messages.createIndex({ createdAt: -1 });
print("✓ Created index: messages.createdAt");

db.messages.createIndex({ recipientEmail: 1 });
print("✓ Created index: messages.recipientEmail");

// ============================================================================
// Вставка тестовых данных
// ============================================================================

print("\n--- Inserting test data ---");

// Вставка тестовых пользователей
const users = [
  {
    login: "john_doe",
    email: "john.doe@example.com",
    firstName: "John",
    lastName: "Doe",
    passwordHash: "4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d",
    createdAt: new Date("2024-01-15T10:30:00Z"),
    updatedAt: new Date("2024-01-15T10:30:00Z")
  },
  {
    login: "jane_smith",
    email: "jane.smith@example.com",
    firstName: "Jane",
    lastName: "Smith",
    passwordHash: "4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d",
    createdAt: new Date("2024-01-16T11:45:00Z"),
    updatedAt: new Date("2024-01-16T11:45:00Z")
  },
  {
    login: "bob_wilson",
    email: "bob.wilson@example.com",
    firstName: "Bob",
    lastName: "Wilson",
    passwordHash: "4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d",
    createdAt: new Date("2024-01-17T09:15:00Z"),
    updatedAt: new Date("2024-01-17T09:15:00Z")
  },
  {
    login: "alice_johnson",
    email: "alice.johnson@example.com",
    firstName: "Alice",
    lastName: "Johnson",
    passwordHash: "4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d",
    createdAt: new Date("2024-01-18T14:20:00Z"),
    updatedAt: new Date("2024-01-18T14:20:00Z")
  },
  {
    login: "charlie_brown",
    email: "charlie.brown@example.com",
    firstName: "Charlie",
    lastName: "Brown",
    passwordHash: "4dbd5e49147b5102ee2731ac03dd0db7decc3b8715c3df3c1f3ddc62dcbcf86d",
    createdAt: new Date("2024-01-19T16:30:00Z"),
    updatedAt: new Date("2024-01-19T16:30:00Z")
  }
];

let userResult;
try {
  userResult = db.users.insertMany(users);
  // insertedCount may be undefined in newer MongoDB versions, get count from insertedIds
  const userCount = userResult.insertedCount || Object.keys(userResult.insertedIds).length;
  print(`✓ Inserted ${userCount} users`);
} catch (e) {
  // If insert fails due to duplicates, get existing users
  print("✓ Users already exist, using existing data");
  const existingUsers = db.users.find().limit(5).toArray();
  userResult = {
    insertedIds: {}
  };
  existingUsers.forEach((u, i) => { userResult.insertedIds[i] = u._id; });
}
print("User result: " + JSON.stringify(userResult));

// Функция для вычисления внешнего numeric ID из ObjectId (FNV-1a хеширование)
// Вычисляем ObjectId массив для всех пользователей
print("userResult: " + JSON.stringify(userResult));
const userCount = userResult.insertedCount || Object.keys(userResult.insertedIds).length;
print("userCount: " + userCount);
const userObjIds = [];
for (let i = 0; i < userCount; i++) {
    const userObjId = userResult.insertedIds[i];
    print("Processing user " + i + ": " + userObjId.valueOf());
    userObjIds.push(userObjId);
}
print("userObjIds computed: " + JSON.stringify(userObjIds.map(o => o.valueOf())));

// Вставка тестовых папок
const folders = [
  { userId: userObjIds[0], name: "Inbox", createdAt: new Date(), updatedAt: new Date() },
  { userId: userObjIds[0], name: "Sent", createdAt: new Date(), updatedAt: new Date() },
  { userId: userObjIds[0], name: "Drafts", createdAt: new Date(), updatedAt: new Date() },
  { userId: userObjIds[1], name: "Inbox", createdAt: new Date(), updatedAt: new Date() },
  { userId: userObjIds[1], name: "Sent", createdAt: new Date(), updatedAt: new Date() },
  { userId: userObjIds[2], name: "Inbox", createdAt: new Date(), updatedAt: new Date() },
  { userId: userObjIds[2], name: "Sent", createdAt: new Date(), updatedAt: new Date() },
  { userId: userObjIds[3], name: "Inbox", createdAt: new Date(), updatedAt: new Date() },
  { userId: userObjIds[3], name: "Sent", createdAt: new Date(), updatedAt: new Date() },
  { userId: userObjIds[4], name: "Inbox", createdAt: new Date(), updatedAt: new Date() }
];

print("Inserting folders...");
let folderResult;
try {
  folderResult = db.folders.insertMany(folders);
  const folderCount = folderResult.insertedCount || Object.keys(folderResult.insertedIds).length;
  print(`✓ Inserted ${folderCount} folders`);
} catch (e) {
  print("Error inserting folders: " + e.message);
  print("Error code: " + e.code);
  print("Error details: " + JSON.stringify(e.writeErrors));
  throw e;
}

// Вычисляем ObjectId массив для всех папок (для использования с $oid в MongoDB запросах)
const folderObjIds = [];
if (folderResult && folderResult.insertedIds) {
  const folderCount = folderResult.insertedCount || Object.keys(folderResult.insertedIds).length;
  for (let i = 0; i < folderCount; i++) {
      const folderObjId = folderResult.insertedIds[i];
      // Store ObjectId directly for use with $oid in queries
      folderObjIds.push(folderObjId);
  }
}
print("folderObjIds computed: " + JSON.stringify(folderObjIds.map(o => o.valueOf())));

// Вставка тестовых писем
const messages = [
  {
    folderId: folderObjIds[0],
    sender: {
      _id: userObjIds[1],
      login: "jane_smith",
      email: "jane.smith@example.com",
      firstName: "Jane",
      lastName: "Smith"
    },
    recipientEmail: "john.doe@example.com",
    subject: "Meeting tomorrow",
    body: "Hi John, let's meet tomorrow at 10 AM to discuss the project.",
    isSent: false,
    createdAt: new Date(),
    updatedAt: new Date()
  },
  {
    folderId: folderObjIds[0],
    sender: {
      _id: userObjIds[2],
      login: "bob_wilson",
      email: "bob.wilson@example.com",
      firstName: "Bob",
      lastName: "Wilson"
    },
    recipientEmail: "john.doe@example.com",
    subject: "Project update",
    body: "John, I've completed the first phase of the project. Please review.",
    isSent: false,
    createdAt: new Date(),
    updatedAt: new Date()
  },
  {
    folderId: folderObjIds[1],
    sender: {
      _id: userObjIds[0],
      login: "john_doe",
      email: "john.doe@example.com",
      firstName: "John",
      lastName: "Doe"
    },
    recipientEmail: "jane.smith@example.com",
    subject: "Re: Meeting tomorrow",
    body: "Sure Jane, 10 AM works for me. See you then!",
    isSent: true,
    createdAt: new Date(),
    updatedAt: new Date()
  },
  {
    folderId: folderObjIds[3],
    sender: {
      _id: userObjIds[0],
      login: "john_doe",
      email: "john.doe@example.com",
      firstName: "John",
      lastName: "Doe"
    },
    recipientEmail: "jane.smith@example.com",
    subject: "Re: Meeting tomorrow",
    body: "Sure Jane, 10 AM works for me. See you then!",
    isSent: false,
    createdAt: new Date(),
    updatedAt: new Date()
  },
  {
    folderId: folderObjIds[5],
    sender: {
      _id: userObjIds[0],
      login: "john_doe",
      email: "john.doe@example.com",
      firstName: "John",
      lastName: "Doe"
    },
    recipientEmail: "bob.wilson@example.com",
    subject: "Re: Project update",
    body: "Thanks Bob! I'll review it today.",
    isSent: false,
    createdAt: new Date(),
    updatedAt: new Date()
  }
];

const messageResult = db.messages.insertMany(messages);
print(`✓ Inserted ${messageResult.insertedCount} messages`);

// ============================================================================
// Статистика
// ============================================================================

print("\n=== Database Statistics ===");
print(`Users: ${db.users.countDocuments()}`);
print(`Folders: ${db.folders.countDocuments()}`);
print(`Messages: ${db.messages.countDocuments()}`);

print("\n=== Indexes ===");
print("Users indexes:");
db.users.getIndexes().forEach(index => {
  print(`  - ${index.name}: ${JSON.stringify(index.key)}`);
});

print("\nFolders indexes:");
db.folders.getIndexes().forEach(index => {
  print(`  - ${index.name}: ${JSON.stringify(index.key)}`);
});

print("\nMessages indexes:");
db.messages.getIndexes().forEach(index => {
  print(`  - ${index.name}: ${JSON.stringify(index.key)}`);
});

print("\n=== MongoDB Initialization Completed Successfully! ===");
print("Database is ready for use.");
