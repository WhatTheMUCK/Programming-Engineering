# Event Catalog - Каталог событий Email Service

Этот документ содержит полный каталог всех событий, публикуемых в Email Service Event-Driven архитектуре.

---

## 1. UserCreated

### Описание
Событие публикуется при создании нового пользователя в системе.

### Команда
```
POST /api/v1/users
```

### Производитель
**User Service** (`src/user/handlers.cpp` → `CreateUserHandler`)

### Потребители
- Notification Service (отправка приветственного письма)
- Analytics Service (сбор статистики по новым пользователям)
- Audit Service (логирование создания пользователя)

### Структура Payload

```json
{
  "event_id": "550e8400-e29b-41d4-a716-446655440000",
  "event_type": "UserCreated",
  "timestamp": "2024-05-03T11:10:00Z",
  "version": "1.0",
  "data": {
    "user_id": "507f1f77bcf86cd799439011",
    "login": "john_doe",
    "email": "john@example.com",
    "first_name": "John",
    "last_name": "Doe",
    "created_at": "2024-05-03T11:10:00Z"
  }
}
```

### Поля Payload

| Поле | Тип | Описание |
|------|-----|---------|
| event_id | UUID | Уникальный идентификатор события |
| event_type | string | Тип события: "UserCreated" |
| timestamp | ISO8601 | Время создания события |
| version | string | Версия схемы события |
| data.user_id | ObjectId | MongoDB ObjectId пользователя |
| data.login | string | Логин пользователя |
| data.email | string | Email пользователя |
| data.first_name | string | Имя пользователя |
| data.last_name | string | Фамилия пользователя |
| data.created_at | ISO8601 | Время создания пользователя |

### Гарантии доставки
**at-least-once** - событие будет доставлено минимум один раз

### Обработка ошибок
Если consumer не может обработать событие, оно возвращается в очередь для повторной попытки.

### Пример использования в коде

```cpp
// Producer
producer_.PublishUserCreated(created_user);

// Consumer
void Process(std::string message) override {
    auto json = formats::json::FromString(message);
    if (json["event_type"].As<std::string>() == "UserCreated") {
        auto user_id = json["data"]["user_id"].As<std::string>();
        // Отправить приветственное письмо
        SendWelcomeEmail(user_id);
    }
}
```

---

## 2. UserUpdated

### Описание
Событие публикуется при обновлении данных пользователя.

### Команда
```
PUT /api/v1/users/{user_id}
```

### Производитель
**User Service** (`src/user/handlers.cpp` → `UpdateUserHandler`)

### Потребители
- Audit Service (логирование изменений)
- Analytics Service (сбор статистики по обновлениям)

### Структура Payload

```json
{
  "event_id": "550e8400-e29b-41d4-a716-446655440001",
  "event_type": "UserUpdated",
  "timestamp": "2024-05-03T11:11:00Z",
  "version": "1.0",
  "data": {
    "user_id": "507f1f77bcf86cd799439011",
    "login": "john_doe",
    "email": "john.doe@example.com",
    "first_name": "John",
    "last_name": "Doe",
    "updated_at": "2024-05-03T11:11:00Z",
    "changed_fields": ["email"]
  }
}
```

### Поля Payload

| Поле | Тип | Описание |
|------|-----|---------|
| event_id | UUID | Уникальный идентификатор события |
| event_type | string | Тип события: "UserUpdated" |
| timestamp | ISO8601 | Время создания события |
| version | string | Версия схемы события |
| data.user_id | ObjectId | MongoDB ObjectId пользователя |
| data.login | string | Логин пользователя |
| data.email | string | Email пользователя |
| data.first_name | string | Имя пользователя |
| data.last_name | string | Фамилия пользователя |
| data.updated_at | ISO8601 | Время обновления пользователя |
| data.changed_fields | array | Список измененных полей |

### Гарантии доставки
**at-least-once** - событие будет доставлено минимум один раз

### Обработка ошибок
Если consumer не может обработать событие, оно возвращается в очередь для повторной попытки.

---

## 3. FolderCreated

### Описание
Событие публикуется при создании новой папки пользователем.

### Команда
```
POST /api/v1/folders
```

### Производитель
**Folder Service** (`src/folder/handlers.cpp` → `CreateFolderHandler`)

### Потребители
- Analytics Service (сбор статистики по папкам)
- Audit Service (логирование создания папки)

### Структура Payload

```json
{
  "event_id": "550e8400-e29b-41d4-a716-446655440002",
  "event_type": "FolderCreated",
  "timestamp": "2024-05-03T11:12:00Z",
  "version": "1.0",
  "data": {
    "folder_id": "507f1f77bcf86cd799439012",
    "user_id": "507f1f77bcf86cd799439011",
    "name": "Inbox",
    "created_at": "2024-05-03T11:12:00Z"
  }
}
```

### Поля Payload

| Поле | Тип | Описание |
|------|-----|---------|
| event_id | UUID | Уникальный идентификатор события |
| event_type | string | Тип события: "FolderCreated" |
| timestamp | ISO8601 | Время создания события |
| version | string | Версия схемы события |
| data.folder_id | ObjectId | MongoDB ObjectId папки |
| data.user_id | ObjectId | MongoDB ObjectId пользователя |
| data.name | string | Название папки |
| data.created_at | ISO8601 | Время создания папки |

### Гарантии доставки
**at-least-once** - событие будет доставлено минимум один раз

### Обработка ошибок
Если consumer не может обработать событие, оно возвращается в очередь для повторной попытки.

---

## 4. MessageCreated

### Описание
Событие публикуется при создании нового сообщения в папке.

### Команда
```
POST /api/v1/folders/{folder_id}/messages
```

### Производитель
**Message Service** (`src/message/handlers.cpp` → `CreateMessageHandler`)

### Потребители
- Search Service (индексация сообщений для полнотекстового поиска)
- Analytics Service (сбор статистики по сообщениям)
- Audit Service (логирование создания сообщения)

### Структура Payload

```json
{
  "event_id": "550e8400-e29b-41d4-a716-446655440003",
  "event_type": "MessageCreated",
  "timestamp": "2024-05-03T11:13:00Z",
  "version": "1.0",
  "data": {
    "message_id": "507f1f77bcf86cd799439013",
    "folder_id": "507f1f77bcf86cd799439012",
    "sender_id": "507f1f77bcf86cd799439011",
    "recipient_email": "recipient@example.com",
    "subject": "Test Subject",
    "body": "Test message body",
    "is_sent": false,
    "created_at": "2024-05-03T11:13:00Z"
  }
}
```

### Поля Payload

| Поле | Тип | Описание |
|------|-----|---------|
| event_id | UUID | Уникальный идентификатор события |
| event_type | string | Тип события: "MessageCreated" |
| timestamp | ISO8601 | Время создания события |
| version | string | Версия схемы события |
| data.message_id | ObjectId | MongoDB ObjectId сообщения |
| data.folder_id | ObjectId | MongoDB ObjectId папки |
| data.sender_id | ObjectId | MongoDB ObjectId отправителя |
| data.recipient_email | string | Email получателя |
| data.subject | string | Тема сообщения |
| data.body | string | Текст сообщения |
| data.is_sent | boolean | Отправлено ли сообщение |
| data.created_at | ISO8601 | Время создания сообщения |

### Гарантии доставки
**at-least-once** - событие будет доставлено минимум один раз

### Обработка ошибок
Если consumer не может обработать событие, оно возвращается в очередь для повторной попытки.

---

## 5. MessageSent

### Описание
Событие публикуется при отправке сообщения внешнему получателю.

### Команда
```
PUT /api/v1/messages/{message_id}/send
```

### Производитель
**Message Service** (`src/message/handlers.cpp` → `SendMessageHandler`)

### Потребители
- Notification Service (отправка уведомления получателю)
- Analytics Service (сбор статистики по отправкам)
- Audit Service (логирование отправки)

### Структура Payload

```json
{
  "event_id": "550e8400-e29b-41d4-a716-446655440004",
  "event_type": "MessageSent",
  "timestamp": "2024-05-03T11:14:00Z",
  "version": "1.0",
  "data": {
    "message_id": "507f1f77bcf86cd799439013",
    "folder_id": "507f1f77bcf86cd799439012",
    "sender_id": "507f1f77bcf86cd799439011",
    "recipient_email": "recipient@example.com",
    "subject": "Test Subject",
    "sent_at": "2024-05-03T11:14:00Z"
  }
}
```

### Поля Payload

| Поле | Тип | Описание |
|------|-----|---------|
| event_id | UUID | Уникальный идентификатор события |
| event_type | string | Тип события: "MessageSent" |
| timestamp | ISO8601 | Время создания события |
| version | string | Версия схемы события |
| data.message_id | ObjectId | MongoDB ObjectId сообщения |
| data.folder_id | ObjectId | MongoDB ObjectId папки |
| data.sender_id | ObjectId | MongoDB ObjectId отправителя |
| data.recipient_email | string | Email получателя |
| data.subject | string | Тема сообщения |
| data.sent_at | ISO8601 | Время отправки сообщения |

### Гарантии доставки
**at-least-once** - событие будет доставлено минимум один раз

### Обработка ошибок
Если consumer не может обработать событие, оно возвращается в очередь для повторной попытки.

### Пример использования в коде

```cpp
// Producer
producer_.PublishMessageSent(message);

// Consumer
void Process(std::string message) override {
    auto json = formats::json::FromString(message);
    if (json["event_type"].As<std::string>() == "MessageSent") {
        auto recipient = json["data"]["recipient_email"].As<std::string>();
        // Отправить уведомление получателю
        SendNotificationEmail(recipient);
    }
}
```

---

## 6. MessageRead

### Описание
Событие публикуется при прочтении сообщения пользователем.

### Команда
```
PUT /api/v1/messages/{message_id}/read
```

### Производитель
**Message Service** (`src/message/handlers.cpp` → `MarkAsReadHandler`)

### Потребители
- Analytics Service (сбор статистики по прочтению)
- Audit Service (логирование прочтения)

### Структура Payload

```json
{
  "event_id": "550e8400-e29b-41d4-a716-446655440005",
  "event_type": "MessageRead",
  "timestamp": "2024-05-03T11:15:00Z",
  "version": "1.0",
  "data": {
    "message_id": "507f1f77bcf86cd799439013",
    "folder_id": "507f1f77bcf86cd799439012",
    "user_id": "507f1f77bcf86cd799439011",
    "read_at": "2024-05-03T11:15:00Z"
  }
}
```

### Поля Payload

| Поле | Тип | Описание |
|------|-----|---------|
| event_id | UUID | Уникальный идентификатор события |
| event_type | string | Тип события: "MessageRead" |
| timestamp | ISO8601 | Время создания события |
| version | string | Версия схемы события |
| data.message_id | ObjectId | MongoDB ObjectId сообщения |
| data.folder_id | ObjectId | MongoDB ObjectId папки |
| data.user_id | ObjectId | MongoDB ObjectId пользователя |
| data.read_at | ISO8601 | Время прочтения сообщения |

### Гарантии доставки
**at-least-once** - событие будет доставлено минимум один раз

### Обработка ошибок
Если consumer не может обработать событие, оно возвращается в очередь для повторной попытки.

---

## 7. Таблица сравнения событий

| Событие | Производитель | Потребители | Тип | Гарантия |
|---------|---------------|-------------|-----|----------|
| UserCreated | User Service | Notification, Analytics, Audit | Domain | at-least-once |
| UserUpdated | User Service | Audit, Analytics | Domain | at-least-once |
| FolderCreated | Folder Service | Analytics, Audit | Domain | at-least-once |
| MessageCreated | Message Service | Search, Analytics, Audit | Domain | at-least-once |
| MessageSent | Message Service | Notification, Analytics, Audit | Domain | at-least-once |
| MessageRead | Message Service | Analytics, Audit | Domain | at-least-once |

---

## 8. Топология RabbitMQ

### Exchange

```
Name: email-events
Type: fanout
Durable: true
Auto-delete: false
```

### Queue

```
Name: email-events-queue
Durable: true
Auto-delete: false
Max length: unlimited
```

### Binding

```
Exchange: email-events
Queue: email-events-queue
Routing key: (не используется для fanout)
```

### Consumers

```
Queue: email-events-queue
Consumers:
  - Notification Service (prefetch: 5)
  - Analytics Service (prefetch: 5)
  - Audit Service (prefetch: 5)
  - Search Service (prefetch: 5)
```

---

## 9. Версионирование событий

### Текущая версия
Все события используют версию **1.0**

### Стратегия версионирования
При изменении структуры события:
1. Добавить новое поле в payload
2. Увеличить minor версию (1.0 → 1.1)
3. Consumers должны быть совместимы с обеими версиями

### Пример обновления

```json
// Версия 1.0
{
  "event_type": "UserCreated",
  "version": "1.0",
  "data": {
    "user_id": "...",
    "login": "...",
    "email": "..."
  }
}

// Версия 1.1 (добавлено новое поле)
{
  "event_type": "UserCreated",
  "version": "1.1",
  "data": {
    "user_id": "...",
    "login": "...",
    "email": "...",
    "phone": "..."  // Новое поле
  }
}
```

---

## 10. Обработка ошибок и retry логика

### Retry стратегия

1. **Первая попытка:** Сразу после получения события
2. **Вторая попытка:** Через 5 секунд
3. **Третья попытка:** Через 30 секунд
4. **Четвертая попытка:** Через 5 минут
5. **Dead Letter Queue:** После 4 неудачных попыток

### Dead Letter Queue

```
Name: email-events-dlq
Purpose: Хранение событий, которые не удалось обработать
Retention: 7 дней
```

### Мониторинг

Все события в DLQ должны быть проанализированы и обработаны вручную.

---

## 11. Примеры использования

### Пример 1: Создание пользователя

```bash
# Request
curl -X POST http://localhost:8081/api/v1/users \
  -H "Content-Type: application/json" \
  -d '{
    "login": "john_doe",
    "email": "john@example.com",
    "first_name": "John",
    "last_name": "Doe",
    "password": "securePassword123"
  }'

# Response
{
  "id": "507f1f77bcf86cd799439011",
  "login": "john_doe",
  "email": "john@example.com",
  "first_name": "John",
  "last_name": "Doe",
  "created_at": "2024-05-03T11:10:00Z"
}

# Event published to RabbitMQ
{
  "event_id": "550e8400-e29b-41d4-a716-446655440000",
  "event_type": "UserCreated",
  "timestamp": "2024-05-03T11:10:00Z",
  "version": "1.0",
  "data": {
    "user_id": "507f1f77bcf86cd799439011",
    "login": "john_doe",
    "email": "john@example.com",
    "first_name": "John",
    "last_name": "Doe",
    "created_at": "2024-05-03T11:10:00Z"
  }
}
```

### Пример 2: Отправка сообщения

```bash
# Request
curl -X PUT http://localhost:8083/api/v1/messages/507f1f77bcf86cd799439013/send \
  -H "Content-Type: application/json"

# Response
{
  "id": "507f1f77bcf86cd799439013",
  "folder_id": "507f1f77bcf86cd799439012",
  "sender_id": "507f1f77bcf86cd799439011",
  "recipient_email": "recipient@example.com",
  "subject": "Test Subject",
  "body": "Test message body",
  "is_sent": true,
  "created_at": "2024-05-03T11:13:00Z"
}

# Event published to RabbitMQ
{
  "event_id": "550e8400-e29b-41d4-a716-446655440004",
  "event_type": "MessageSent",
  "timestamp": "2024-05-03T11:14:00Z",
  "version": "1.0",
  "data": {
    "message_id": "507f1f77bcf86cd799439013",
    "folder_id": "507f1f77bcf86cd799439012",
    "sender_id": "507f1f77bcf86cd799439011",
    "recipient_email": "recipient@example.com",
    "subject": "Test Subject",
    "sent_at": "2024-05-03T11:14:00Z"
  }
}
```

---

## 12. Заключение

Этот каталог событий определяет все события, публикуемые в Email Service Event-Driven архитектуре. Каждое событие имеет четко определенную структуру, производителя, потребителей и гарантии доставки.

Все события используют **at-least-once** гарантию доставки, что означает, что consumers должны быть идемпотентными и готовы обрабатывать одно и то же событие несколько раз.

Версионирование событий позволяет эволюционировать систему без нарушения совместимости с существующими consumers.
