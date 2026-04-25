# MongoDB Schema Design for Email Service

## Домашнее задание 4: Проектирование и работа с MongoDB

**Дата создания:** 2026-04-18  
**Вариант:** 9 - Электронная почта  
**СУБД:** MongoDB 7.0  
**Подход:** Гибридный (Hybrid)  

---

## Оглавление

1. [Обзор архитектуры](#обзор-архитектуры)
2. [Проектирование коллекций](#проектирование-коллекций)
3. [Обоснование выбора подхода](#обоснование-выбора-подхода)
4. [Сравнение подходов](#сравнение-подходов)
5. [Стратегия индексирования](#стратегия-индексирования)
6. [Примеры запросов](#примеры-запросов)
7. [Валидация схем](#валидация-схем)

---

## Обзор архитектуры

### ER-диаграмма MongoDB коллекций

```
┌─────────────────────────────────────────────────────────────┐
│                      MongoDB 7.0                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  users (коллекция)                                          │
│  ├─ _id (ObjectId PK)                                       │
│  ├─ login (UNIQUE)                                          │
│  ├─ email (UNIQUE)                                          │
│  ├─ firstName, lastName                                     │
│  ├─ passwordHash                                            │
│  └─ createdAt, updatedAt                                    │
│                                                             │
│  folders (коллекция)                                        │
│  ├─ _id (ObjectId PK)                                       │
│  ├─ userId (ObjectId) ──► users._id (reference)             │
│  ├─ name                                                    │
│  └─ createdAt, updatedAt                                    │
│                                                             │
│  messages (коллекция)                                       │
│  ├─ _id (ObjectId PK)                                       │
│  ├─ folderId (ObjectId) ──► folders._id (reference)         │
│  ├─ sender (EMBEDDED: {_id, login, email, firstName...})    │
│  ├─ recipientEmail                                          │
│  ├─ subject, body                                           │
│  ├─ isSent                                                  │
│  └─ createdAt, updatedAt                                    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Связи между коллекциями

| Связь | Тип | Обоснование |
|-------|-----|-------------|
| `folders.userId` → `users._id` | Reference (ObjectId) | Папки принадлежат пользователям, требуется проверка принадлежности |
| `messages.folderId` → `folders._id` | Reference (ObjectId) | Письма находятся в папках, поддержка перемещения между папками |
| `messages.sender` (embedded) | Embedded | Информация об отправителе встроена для быстрого чтения писем |

---

## Проектирование коллекций

### Коллекция `users`

#### Структура документа

```javascript
{
  _id: ObjectId("507f1f77bcf86cd799439011"),
  login: "john_doe",
  email: "john@example.com",
  firstName: "John",
  lastName: "Doe",
  passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8",
  createdAt: ISODate("2024-01-15T10:30:00Z"),
  updatedAt: ISODate("2024-01-15T10:30:00Z")
}
```

#### Индексы

| Индекс | Тип | Обоснование |
|--------|-----|-------------|
| `{ login: 1 }` | UNIQUE | Быстрый поиск по логину (Q2: FindUserByLogin) |
| `{ email: 1 }` | UNIQUE | Проверка уникальности email |
| `{ firstName: 1, lastName: 1 }` | COMPOSITE | Поиск по маске имени (Q3: SearchUserByNameMask) |
| `{ createdAt: -1 }` | SINGLE | Сортировка по дате создания |

#### Валидация схемы

```javascript
{
  $jsonSchema: {
    bsonType: "object",
    required: ["login", "email", "firstName", "lastName", "passwordHash"],
    properties: {
      login: {
        bsonType: "string",
        minLength: 3,
        maxLength: 255,
        description: "User login (unique)"
      },
      email: {
        bsonType: "string",
        pattern: "^[^@]+@[^@]+$",
        description: "User email (unique)"
      },
      firstName: {
        bsonType: "string",
        minLength: 1,
        maxLength: 255,
        description: "User first name"
      },
      lastName: {
        bsonType: "string",
        minLength: 1,
        maxLength: 255,
        description: "User last name"
      },
      passwordHash: {
        bsonType: "string",
        minLength: 64,
        description: "SHA-256 hash of password"
      },
      createdAt: {
        bsonType: "date",
        description: "Creation timestamp"
      },
      updatedAt: {
        bsonType: "date",
        description: "Last update timestamp"
      }
    }
  }
}
```

---

### Коллекция `folders`

#### Структура документа

```javascript
{
  _id: ObjectId("507f1f77bcf86cd799439012"),
  userId: ObjectId("507f1f77bcf86cd799439011"),  // Reference to users._id
  name: "Inbox",
  createdAt: ISODate("2024-01-15T10:30:00Z"),
  updatedAt: ISODate("2024-01-15T10:30:00Z")
}
```

> **Примечание:** `userId` — поле типа `ObjectId`, ссылающееся на документ пользователя в коллекции `users`. Это обеспечивает целостность данных и позволяет легко проверять принадлежность папки пользователю.

#### Индексы

| Индекс | Тип | Обоснование |
|--------|-----|-------------|
| `{ userId: 1, name: 1 }` | UNIQUE | Проверка уникальности папок пользователя |
| `{ userId: 1 }` | SINGLE | Получение всех папок пользователя (Q5: ListFolders) |
| `{ createdAt: -1 }` | SINGLE | Сортировка по дате создания |

#### Валидация схемы

```javascript
{
  $jsonSchema: {
    bsonType: "object",
    required: ["userId", "name"],
    properties: {
      userId: {
        bsonType: "objectId",
        description: "Reference to user document"
      },
      name: {
        bsonType: "string",
        minLength: 1,
        maxLength: 255,
        description: "Folder name"
      },
      createdAt: {
        bsonType: "date",
        description: "Creation timestamp"
      },
      updatedAt: {
        bsonType: "date",
        description: "Last update timestamp"
      }
    }
  }
}
```

---

### Коллекция `messages`

#### Структура документа

```javascript
{
  _id: ObjectId("507f1f77bcf86cd799439013"),
  folderId: ObjectId("507f1f77bcf86cd799439012"),  // Reference to folders._id
  sender: {
    _id: ObjectId("507f1f77bcf86cd799439011"),  // ObjectId для быстрого поиска
    login: "john_doe",
    email: "john@example.com",
    firstName: "John",
    lastName: "Doe"
  },
  recipientEmail: "jane@example.com",
  subject: "Meeting tomorrow",
  body: "Let's discuss the project details tomorrow at 10 AM.",
  isSent: true,
  createdAt: ISODate("2024-01-15T10:30:00Z"),
  updatedAt: ISODate("2024-01-15T10:30:00Z")
}
```

> **Примечание:** Поле `sender` содержит встроенный (embedded) документ с информацией об отправителе. Это позволяет избежать дополнительного запроса к коллекции `users` при чтении писем. Поле `sender._id` остаётся `ObjectId` для возможности поиска писем от конкретного отправителя.

#### Индексы

| Индекс | Тип | Обоснование |
|--------|-----|-------------|
| `{ folderId: 1, createdAt: -1 }` | COMPOSITE | Получение писем в папке с сортировкой (Q7: ListMessagesInFolder) |
| `{ folderId: 1 }` | SINGLE | Быстрый поиск по папке |
| `{ "sender._id": 1 }` | SINGLE | Поиск писем от конкретного отправителя |
| `{ createdAt: -1 }` | SINGLE | Сортировка по дате |
| `{ recipientEmail: 1 }` | SINGLE | Поиск писем по адресу получателя |

#### Валидация схемы

```javascript
{
  $jsonSchema: {
    bsonType: "object",
    required: ["folderId", "sender", "recipientEmail", "subject", "body"],
    properties: {
      folderId: {
        bsonType: "objectId",
        description: "Reference to folder document"
      },
      sender: {
        bsonType: "object",
        required: ["_id", "login", "email"],
        properties: {
          _id: {
            bsonType: "objectId",
            description: "MongoDB ObjectId of sender user"
          },
          login: {
            bsonType: "string",
            description: "Sender login"
          },
          email: {
            bsonType: "string",
            description: "Sender email"
          },
          firstName: {
            bsonType: "string",
            description: "Sender first name (optional)"
          },
          lastName: {
            bsonType: "string",
            description: "Sender last name (optional)"
          }
        }
      },
      recipientEmail: {
        bsonType: "string",
        pattern: "^[^@]+@[^@]+$",
        description: "Recipient email address"
      },
      subject: {
        bsonType: "string",
        minLength: 1,
        maxLength: 500,
        description: "Message subject"
      },
      body: {
        bsonType: "string",
        minLength: 1,
        description: "Message body"
      },
      isSent: {
        bsonType: "bool",
        description: "Whether message has been sent"
      },
      createdAt: {
        bsonType: "date",
        description: "Creation timestamp"
      },
      updatedAt: {
        bsonType: "date",
        description: "Last update timestamp"
      }
    }
  }
}
```

---

## Обоснование выбора подхода

### Почему гибридный подход?

Гибридный подход (Hybrid) сочетает преимущества денормализованного и нормализованного подходов:

1. **Embedded sender в messages**: 
   - Письма часто читаются вместе с информацией об отправителе
   - Избегает JOIN операций при получении писем
   - Улучшает производительность чтения (Q7, Q8)

2. **Reference для folders и users**:
   - Пользователь может иметь много папок
   - Папки редко обновляются
   - Сохраняет консистентность данных
   - Легко проверить принадлежность папки пользователю

3. **Reference для messages и folders**:
   - Папка может содержать много писем
   - Письма могут перемещаться между папками
   - Гибкость в управлении письмами

### Преимущества для email-сервиса

| Аспект | Преимущество |
|--------|-------------|
| **Производительность чтения** | Письма читаются с информацией об отправителе за 1 запрос |
| **Гибкость схемы** | Легко добавлять новые поля к письмам и папкам |
| **Масштабируемость** | Горизонтальное масштабирование через sharding |
| **Консистентность** | Reference связи обеспечивают целостность данных |
| **Размер документов** | Средний размер документов (не слишком большие) |

---

## Сравнение подходов

### Сравнение производительности операций

| Операция | Денормализованный | Нормализованный | **Гибридный** |
|----------|-------------------|-----------------|---------------|
| Q1: Создание пользователя | 1 запрос | 1 запрос | **1 запрос** ✓ |
| Q2: Поиск по логину | 1 запрос | 1 запрос | **1 запрос** ✓ |
| Q3: Поиск по имени | 1 запрос | 1 запрос | **1 запрос** ✓ |
| Q4: Создание папки | 1 запрос | 1 запрос | **1 запрос** ✓ |
| Q5: Список папок | 1 запрос | 1 запрос | **1 запрос** ✓ |
| Q6: Создание письма | 1 запрос | 1 запрос | **1 запрос** ✓ |
| Q7: Список писем | 1 запрос | 2 запроса (JOIN) | **1 запрос** ✓ |
| Q8: Получить письмо | 1 запрос | 2 запроса (JOIN) | **1 запрос** ✓ |
| Обновление отправителя | 1000+ обновлений | 1 обновление | **1 обновление** ✓ |
| Размер документа | Большой | Маленький | **Средний** ✓ |

### Сравнение характеристик

| Характеристика | Денормализованный | Нормализованный | **Гибридный** |
|----------------|-------------------|-----------------|---------------|
| Производительность чтения | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐ |
| Производительность записи | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| Гибкость схемы | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |
| Консистентность | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| Размер документов | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |

**Вывод:** Гибридный подход обеспечивает оптимальный баланс между производительностью и консистентностью для email-сервиса.

---

## Стратегия индексирования

### Индексы для коллекции `users`

```javascript
// Уникальный индекс для логина
db.users.createIndex({ login: 1 }, { unique: true });

// Уникальный индекс для email
db.users.createIndex({ email: 1 }, { unique: true });

// Составной индекс для поиска по имени
db.users.createIndex({ firstName: 1, lastName: 1 });

// Индекс для сортировки по дате создания
db.users.createIndex({ createdAt: -1 });
```

### Индексы для коллекции `folders`

```javascript
// Уникальный составной индекс для проверки уникальности папок пользователя
db.folders.createIndex({ userId: 1, name: 1 }, { unique: true });

// Индекс для получения всех папок пользователя
db.folders.createIndex({ userId: 1 });

// Индекс для сортировки по дате создания
db.folders.createIndex({ createdAt: -1 });
```

### Индексы для коллекции `messages`

```javascript
// Составной индекс для получения писем в папке с сортировкой
db.messages.createIndex({ folderId: 1, createdAt: -1 });

// Индекс для быстрого поиска по папке
db.messages.createIndex({ folderId: 1 });

// Индекс для поиска писем от конкретного отправителя
db.messages.createIndex({ "sender._id": 1 });

// Индекс для сортировки по дате
db.messages.createIndex({ createdAt: -1 });

// Индекс для поиска писем по адресу получателя
db.messages.createIndex({ recipientEmail: 1 });
```

### Обоснование индексов

| Индекс | Операция | Частота использования |
|--------|----------|----------------------|
| `users.login` | Q2: FindUserByLogin | Высокая |
| `users.email` | Проверка уникальности | Высокая |
| `users.firstName, lastName` | Q3: SearchUserByNameMask | Средняя |
| `folders.userId, name` | Проверка уникальности | Высокая |
| `folders.userId` | Q5: ListFolders | Высокая |
| `messages.folderId, createdAt` | Q7: ListMessagesInFolder | Высокая |
| `messages.folderId` | Q8: GetMessageById | Высокая |
| `messages.sender._id` | Поиск по отправителю | Средняя |

---

## Примеры запросов

### Q1: Создание пользователя

```javascript
db.users.insertOne({
  login: "john_doe",
  email: "john@example.com",
  firstName: "John",
  lastName: "Doe",
  passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8",
  createdAt: new Date(),
  updatedAt: new Date()
});
```

### Q2: Поиск пользователя по логину

```javascript
db.users.findOne(
  { login: "john_doe" },
  { projection: { passwordHash: 0 } }
);
```

### Q3: Поиск пользователя по маске имени

```javascript
db.users.find({
  $or: [
    { firstName: { $regex: "John", $options: "i" } },
    { lastName: { $regex: "Doe", $options: "i" } }
  ]
}).toArray();
```

### Q4: Создание папки

```javascript
db.folders.insertOne({
  userId: ObjectId("507f1f77bcf86cd799439011"),
  name: "Inbox",
  createdAt: new Date(),
  updatedAt: new Date()
});
```

### Q5: Получение папок пользователя

```javascript
db.folders.find({
  userId: ObjectId("507f1f77bcf86cd799439011")
}).sort({ createdAt: -1 }).toArray();
```

### Q6: Создание письма

```javascript
db.messages.insertOne({
  folderId: ObjectId("507f1f77bcf86cd799439012"),
  sender: {
    _id: ObjectId("507f1f77bcf86cd799439011"),
    login: "john_doe",
    email: "john@example.com",
    firstName: "John",
    lastName: "Doe"
  },
  recipientEmail: "jane@example.com",
  subject: "Meeting tomorrow",
  body: "Let's discuss the project details tomorrow at 10 AM.",
  isSent: true,
  createdAt: new Date(),
  updatedAt: new Date()
});
```

### Q7: Получение писем в папке

```javascript
db.messages.find({
  folderId: ObjectId("507f1f77bcf86cd799439012")
}).sort({ createdAt: -1 }).toArray();
```

### Q8: Получение письма по ID

```javascript
db.messages.findOne({
  _id: ObjectId("507f1f77bcf86cd799439013")
});
```

---

## Валидация схем

### Проверка обязательных полей

```javascript
// Попытка вставить пользователя без обязательного поля
db.users.insertOne({
  login: "test_user",
  email: "test@example.com"
  // Отсутствуют: firstName, lastName, passwordHash
});
// Результат: Ошибка валидации
```

### Проверка типов данных

```javascript
// Попытка вставить пользователя с неверным типом
db.users.insertOne({
  login: "test_user",
  email: "test@example.com",
  firstName: "Test",
  lastName: "User",
  passwordHash: 12345  // Должен быть строкой
});
// Результат: Ошибка валидации
```

### Проверка ограничений

```javascript
// Попытка вставить пользователя с коротким логином
db.users.insertOne({
  login: "ab",  // Минимум 3 символа
  email: "test@example.com",
  firstName: "Test",
  lastName: "User",
  passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8"
});
// Результат: Ошибка валидации
```

### Проверка UNIQUE индексов

```javascript
// Попытка вставить дубликат логина
db.users.insertOne({
  login: "john_doe",  // Уже существует
  email: "another@example.com",
  firstName: "John",
  lastName: "Smith",
  passwordHash: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8"
});
// Результат: Ошибка дубликата ключа
```

---

## Статистика коллекций

### Ожидаемый размер данных

| Коллекция | Количество документов | Средний размер документа | Общий размер |
|-----------|----------------------|-------------------------|--------------|
| users | 15+ | ~500 bytes | ~7.5 KB |
| folders | 35+ | ~200 bytes | ~7 KB |
| messages | 50+ | ~1 KB | ~50 KB |

### Ожидаемое количество индексов

| Коллекция | Количество индексов | Размер индексов |
|-----------|-------------------|-----------------|
| users | 4 | ~2 KB |
| folders | 3 | ~1.5 KB |
| messages | 5 | ~5 KB |

---

## Заключение

Гибридный подход к проектированию MongoDB схемы для email-сервиса обеспечивает:

1. **Оптимальную производительность**: 1 запрос для большинства операций
2. **Гибкость схемы**: Легко добавлять новые поля
3. **Консистентность данных**: Reference связи для критических данных
4. **Масштабируемость**: Возможность горизонтального масштабирования

Этот подход идеально подходит для email-сервиса, где:
- Письма часто читаются вместе с информацией об отправителе
- Папки редко обновляются
- Пользователи изменяются редко
- Требуется высокая производительность чтения
