// MongoDB Schema Validation Tests for Email Service
// Variant 9: Email Service

db = db.getSiblingDB('email_db');

print("=== SCHEMA VALIDATION TESTS ===\n");

// Create test collection with validation
db.testUsers.drop();
db.createCollection('testUsers', {
  validator: {
    $jsonSchema: {
      bsonType: 'object',
      required: ['login', 'email', 'firstName', 'lastName', 'passwordHash'],
      properties: {
        login: { bsonType: 'string', minLength: 3, maxLength: 255 },
        email: { bsonType: 'string', pattern: '^[^@]+@[^@]+$' },
        firstName: { bsonType: 'string', minLength: 1, maxLength: 255 },
        lastName: { bsonType: 'string', minLength: 1, maxLength: 255 },
        passwordHash: { bsonType: 'string', minLength: 64 },
        createdAt: { bsonType: 'date' }
      }
    }
  }
});
print("Collection created with validation\n");

// Test 1: Valid document
print("Test 1: Insert valid document");
try {
  db.testUsers.insertOne({
    login: "valid_user",
    email: "valid@test.com",
    firstName: "Valid",
    lastName: "User",
    passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8",
    createdAt: new Date()
  });
  print("PASSED: Valid document inserted\n");
} catch (e) {
  print("FAILED: " + e.message + "\n");
}

// Test 2: Missing required field
print("Test 2: Missing required field (no email)");
try {
  db.testUsers.insertOne({
    login: "no_email",
    firstName: "No",
    lastName: "Email",
    passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8"
  });
  print("FAILED: Should have rejected\n");
} catch (e) {
  print("PASSED: Rejected - " + e.codeName + "\n");
}

// Test 3: Wrong type
print("Test 3: Wrong type (passwordHash as number)");
try {
  db.testUsers.insertOne({
    login: "wrong_type",
    email: "wrong@test.com",
    firstName: "Wrong",
    lastName: "Type",
    passwordHash: 12345
  });
  print("FAILED: Should have rejected\n");
} catch (e) {
  print("PASSED: Rejected - " + e.codeName + "\n");
}

// Test 4: String too short
print("Test 4: Login too short (min 3 chars)");
try {
  db.testUsers.insertOne({
    login: "ab",
    email: "short@test.com",
    firstName: "Short",
    lastName: "Login",
    passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8"
  });
  print("FAILED: Should have rejected\n");
} catch (e) {
  print("PASSED: Rejected - " + e.codeName + "\n");
}

// Test 5: Invalid email pattern
print("Test 5: Invalid email pattern");
try {
  db.testUsers.insertOne({
    login: "bad_email",
    email: "not-an-email",
    firstName: "Bad",
    lastName: "Email",
    passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8"
  });
  print("FAILED: Should have rejected\n");
} catch (e) {
  print("PASSED: Rejected - " + e.codeName + "\n");
}

// Test 6: Password hash too short
print("Test 6: Password hash too short (min 64 chars)");
try {
  db.testUsers.insertOne({
    login: "short_hash",
    email: "hash@test.com",
    firstName: "Short",
    lastName: "Hash",
    passwordHash: "abc"
  });
  print("FAILED: Should have rejected\n");
} catch (e) {
  print("PASSED: Rejected - " + e.codeName + "\n");
}

// Test 7: Extra field allowed (flexible schema)
print("Test 7: Extra field allowed (flexible schema)");
try {
  db.testUsers.insertOne({
    login: "extra_field",
    email: "extra@test.com",
    firstName: "Extra",
    lastName: "Field",
    passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8",
    extraField: "allowed"
  });
  print("PASSED: Extra field accepted\n");
} catch (e) {
  print("FAILED: " + e.message + "\n");
}

// Cleanup
db.testUsers.drop();
print("=== VALIDATION TESTS COMPLETE ===");
