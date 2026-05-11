# Event-Driven архитектура Email Service

## 1. Обзор архитектуры

Email Service трансформируется из синхронной архитектуры в Event-Driven архитектуру с использованием RabbitMQ как брокера сообщений. Это позволяет:

- **Слабую связанность** между сервисами
- **Асинхронную обработку** событий
- **Масштабируемость** через добавление новых потребителей событий
- **Надежность** через гарантии доставки сообщений

## 2. Ключевые события в системе

### 2.1 События User Service

#### UserCreated
**Описание:** Событие создания нового пользователя

**Команда:** CreateUser (POST /api/v1/users)

**Структура payload:**
```json
{
  "event_id": "uuid",
  "event_type": "UserCreated",
  "timestamp": "2024-05-03T11:10:00Z",
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

**Производитель:** User Service
**Потенциальные потребители:** 
- Notification Service (отправка приветственного письма)
- Analytics Service (сбор статистики)
- Audit Service (логирование)

**Гарантии доставки:** at-least-once

---

#### UserUpdated
**Описание:** Событие обновления данных пользователя

**Команда:** UpdateUser (PUT /api/v1/users/{user_id})

**Структура payload:**
```json
{
  "event_id": "uuid",
  "event_type": "UserUpdated",
  "timestamp": "2024-05-03T11:10:00Z",
  "data": {
    "user_id": "507f1f77bcf86cd799439011",
    "login": "john_doe",
    "email": "john.doe@example.com",
    "first_name": "John",
    "last_name": "Doe",
    "updated_at": "2024-05-03T11:10:00Z"
  }
}
```

**Производитель:** User Service
**Потенциальные потребители:**
- Notification Service (уведомление об изменении)
- Audit Service (логирование изменений)

**Гарантии доставки:** at-least-once

---

### 2.2 События Folder Service

#### FolderCreated
**Описание:** Событие создания новой папки

**Команда:** CreateFolder (POST /api/v1/folders)

**Структура payload:**
```json
{
  "event_id": "uuid",
  "event_type": "FolderCreated",
  "timestamp": "2024-05-03T11:10:00Z",
  "data": {
    "folder_id": "507f1f77bcf86cd799439012",
    "user_id": "507f1f77bcf86cd799439011",
    "name": "Inbox",
    "created_at": "2024-05-03T11:10:00Z"
  }
}
```

**Производитель:** Folder Service
**Потенциальные потребители:**
- Analytics Service (сбор статистики по папкам)
- Audit Service (логирование)

**Гарантии доставки:** at-least-once

---

### 2.3 События Message Service

#### MessageCreated
**Описание:** Событие создания нового сообщения в папке

**Команда:** CreateMessage (POST /api/v1/folders/{folder_id}/messages)

**Структура payload:**
```json
{
  "event_id": "uuid",
  "event_type": "MessageCreated",
  "timestamp": "2024-05-03T11:10:00Z",
  "data": {
    "message_id": "507f1f77bcf86cd799439013",
    "folder_id": "507f1f77bcf86cd799439012",
    "sender_id": "507f1f77bcf86cd799439011",
    "recipient_email": "recipient@example.com",
    "subject": "Test Subject",
    "body": "Test message body",
    "is_sent": false,
    "created_at": "2024-05-03T11:10:00Z"
  }
}
```

**Производитель:** Message Service
**Потенциальные потребители:**
- Search Service (индексация сообщений)
- Analytics Service (сбор статистики)
- Audit Service (логирование)

**Гарантии доставки:** at-least-once

---

#### MessageSent
**Описание:** Событие отправки сообщения внешнему получателю

**Команда:** SendMessage (PUT /api/v1/messages/{message_id}/send)

**Структура payload:**
```json
{
  "event_id": "uuid",
  "event_type": "MessageSent",
  "timestamp": "2024-05-03T11:10:00Z",
  "data": {
    "message_id": "507f1f77bcf86cd799439013",
    "folder_id": "507f1f77bcf86cd799439012",
    "sender_id": "507f1f77bcf86cd799439011",
    "recipient_email": "recipient@example.com",
    "subject": "Test Subject",
    "sent_at": "2024-05-03T11:10:00Z"
  }
}
```

**Производитель:** Message Service
**Потенциальные потребители:**
- Notification Service (отправка уведомления получателю)
- Analytics Service (сбор статистики отправок)
- Audit Service (логирование)

**Гарантии доставки:** at-least-once

---

#### MessageRead
**Описание:** Событие прочтения сообщения

**Команда:** MarkMessageAsRead (PUT /api/v1/messages/{message_id}/read)

**Структура payload:**
```json
{
  "event_id": "uuid",
  "event_type": "MessageRead",
  "timestamp": "2024-05-03T11:10:00Z",
  "data": {
    "message_id": "507f1f77bcf86cd799439013",
    "folder_id": "507f1f77bcf86cd799439012",
    "user_id": "507f1f77bcf86cd799439011",
    "read_at": "2024-05-03T11:10:00Z"
  }
}
```

**Производитель:** Message Service
**Потенциальные потребители:**
- Analytics Service (сбор статистики прочтения)
- Audit Service (логирование)

**Гарантии доставки:** at-least-once

---

## 3. Архитектура системы

### 3.1 Компоненты системы

```
┌─────────────────────────────────────────────────────────────────┐
│                     Email Service System                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────┐  ┌──────────────────┐  ┌───────────────┐  │
│  │  User Service    │  │ Folder Service   │  │Message Service│  │
│  │                  │  │                  │  │               │  │
│  │ - CreateUser     │  │ - CreateFolder   │  │ - CreateMsg   │  │
│  │ - UpdateUser     │  │                  │  │ - SendMessage │  │
│  │ - GetUser        │  │ - ListFolders    │  │ - GetMessages │  │
│  │ - SearchUser     │  │ - GetFolder      │  │ - ReadMessage │  │
│  └────────┬─────────┘  └────────┬─────────┘  └────────┬──────┘  │
│           │                     │                     │         │
│           │ Publish Events      │ Publish Events      │         │
│           └─────────────────────┼─────────────────────┘         │
│                                 │                               │
│                    ┌────────────▼────────────┐                  │
│                    │   RabbitMQ Broker       │                  │
│                    │                         │                  │
│                    │ Exchange: email-events  │                  │
│                    │ Queue: email-events-q   │                  │
│                    └────────────┬────────────┘                  │
│                                 │                               │
│                    ┌────────────▼────────────┐                  │
│                    │  Event Consumers        │                  │
│                    │                         │                  │
│                    │ - Notification Service  │                  │
│                    │ - Analytics Service     │                  │
│                    │ - Audit Service         │                  │
│                    │ - Search Service        │                  │
│                    └─────────────────────────┘                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 Производители и потребители событий

| Событие | Производитель | Потребители | Тип |
|---------|---------------|-------------|-----|
| UserCreated | User Service | Notification, Analytics, Audit | Domain |
| UserUpdated | User Service | Audit, Analytics | Domain |
| FolderCreated | Folder Service | Analytics, Audit | Domain |
| MessageCreated | Message Service | Search, Analytics, Audit | Domain |
| MessageSent | Message Service | Notification, Analytics, Audit | Domain |
| MessageRead | Message Service | Analytics, Audit | Domain |

---

## 4. Применение паттерна CQRS

### 4.1 Разделение на Commands и Queries

#### Commands (Write операции)
Команды изменяют состояние системы и публикуют события:

```
User Service Commands:
├── CreateUser → публикует UserCreated
└── UpdateUser → публикует UserUpdated

Folder Service Commands:
└── CreateFolder → публикует FolderCreated

Message Service Commands:
├── CreateMessage → публикует MessageCreated
├── SendMessage → публикует MessageSent
└── MarkMessageAsRead → публикует MessageRead
```

#### Queries (Read операции)
Запросы только читают данные, не изменяя состояние:

```
User Service Queries:
├── GetUser
└── SearchUserByNameMask

Folder Service Queries:
├── ListFolders
└── GetFolder

Message Service Queries:
├── GetMessages
└── GetMessageById
```

### 4.2 Синхронизация Read и Write моделей

```
┌─────────────────────────────────────────────────────────────┐
│                    CQRS Pattern                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Write Model (Commands)          Read Model (Queries)       │
│  ┌──────────────────────┐        ┌──────────────────────┐   │
│  │ User Service         │        │ User Read Model      │   │
│  │ - CreateUser         │        │ - GetUser            │   │
│  │ - UpdateUser         │        │ - SearchUser         │   │
│  │                      │        │                      │   │
│  │ MongoDB (Write DB)   │        │ MongoDB (Read DB)    │   │
│  └──────────┬───────────┘        └──────────┬───────────┘   │
│             │                               │               │
│             │ Publish Events                │               │
│             └───────────────────────────────┘               │
│                    via RabbitMQ                             │
│                                                             │
│  Event: UserCreated                                         │
│  {                                                          │
│    "user_id": "...",                                        │
│    "login": "...",                                          │
│    "email": "...",                                          │
│    "first_name": "...",                                     │
│    "last_name": "..."                                       │
│  }                                                          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 4.3 Преимущества CQRS в Email Service

1. **Независимое масштабирование** - read и write модели масштабируются отдельно
2. **Оптимизированные запросы** - read модель может быть оптимизирована для быстрого поиска
3. **Асинхронная синхронизация** - события обновляют read модель асинхронно
4. **Аудит** - все изменения записываются как события

---

## 5. Выбор RabbitMQ

### 5.1 Обоснование выбора

**RabbitMQ выбран вместо Kafka по следующим причинам:**

1. **Простота интеграции** - встроенная поддержка в userver
2. **Надежность** - гарантии доставки сообщений (at-least-once, exactly-once)
3. **Управление** - встроенный Management UI для мониторинга
4. **Производительность** - достаточна для email service
5. **Экосистема** - хорошая документация и примеры

### 5.2 Конфигурация RabbitMQ

```yaml
# RabbitMQ сервис в docker-compose.yaml
rabbitmq:
  image: rabbitmq:3.12-management-alpine
  environment:
    RABBITMQ_DEFAULT_USER: guest
    RABBITMQ_DEFAULT_PASS: guest
    RABBITMQ_DEFAULT_VHOST: /
  ports:
    - "5672:5672"      # AMQP port
    - "15672:15672"    # Management UI
```

### 5.3 Топология RabbitMQ

```
Exchange: email-events
├── Type: fanout (все события идут всем потребителям)
├── Durable: true
└── Auto-delete: false

Queue: email-events-queue
├── Durable: true
├── Auto-delete: false
├── Binding: email-events → email-events-queue
└── Consumers:
    ├── Notification Service
    ├── Analytics Service
    ├── Audit Service
    └── Search Service
```

---

## 6. Гарантии доставки сообщений

### 6.1 At-Least-Once гарантия

**Используется для:** Все события в email service

**Как работает:**
1. Producer публикует сообщение в RabbitMQ
2. RabbitMQ подтверждает получение
3. Consumer получает сообщение
4. Consumer обрабатывает сообщение
5. Consumer отправляет ACK (acknowledgment)
6. RabbitMQ удаляет сообщение из очереди

**Гарантия:** Сообщение будет доставлено минимум один раз

**Потенциальная проблема:** Сообщение может быть обработано несколько раз

**Решение:** Consumers должны быть идемпотентными (обработка одного сообщения несколько раз дает тот же результат)

### 6.2 Реализация в коде

```cpp
// Producer - reliable publishing
client_->PublishReliable(
    exchange_,
    routing_key_,
    envelope,
    deadline
);

// Consumer - manual ACK
void Process(std::string message) override {
    try {
        // Обработка сообщения
        ProcessEvent(message);
        // ACK отправляется автоматически при успехе
    } catch (const std::exception& e) {
        // NACK отправляется при ошибке
        // Сообщение вернется в очередь
        throw;
    }
}
```

---

## 7. Поток событий в системе

### 7.1 Пример: Создание пользователя

```
1. Client отправляет POST /api/v1/users
   ↓
2. User Service получает запрос
   ↓
3. User Service создает пользователя в MongoDB
   ↓
4. User Service публикует UserCreated событие в RabbitMQ
   ↓
5. RabbitMQ доставляет событие потребителям:
   ├─→ Notification Service (отправляет приветственное письмо)
   ├─→ Analytics Service (обновляет статистику)
   └─→ Audit Service (логирует создание)
   ↓
6. User Service возвращает ответ клиенту
```

### 7.2 Пример: Отправка сообщения

```
1. Client отправляет PUT /api/v1/messages/{id}/send
   ↓
2. Message Service получает запрос
   ↓
3. Message Service обновляет статус в MongoDB (is_sent = true)
   ↓
4. Message Service публикует MessageSent событие в RabbitMQ
   ↓
5. RabbitMQ доставляет событие потребителям:
   ├─→ Notification Service (отправляет уведомление получателю)
   ├─→ Analytics Service (обновляет статистику отправок)
   └─→ Audit Service (логирует отправку)
   ↓
6. Message Service возвращает ответ клиенту
```

---

## 8. Компоненты для реализации

### 8.1 EmailEventProducer

**Файл:** `src/common/rabbitmq_producer.hpp`

**Функционал:**
- Публикация событий в RabbitMQ
- Методы для каждого типа события:
  - `PublishUserCreated(const User& user)`
  - `PublishUserUpdated(const User& user)`
  - `PublishFolderCreated(const Folder& folder)`
  - `PublishMessageCreated(const Message& message)`
  - `PublishMessageSent(const Message& message)`
  - `PublishMessageRead(const std::string& message_id, const std::string& user_id)`

### 8.2 EmailEventConsumer

**Файл:** `src/common/rabbitmq_consumer.hpp`

**Функционал:**
- Потребление событий из RabbitMQ
- Парсинг JSON сообщений
- Логирование потребленных событий
- Хранение обработанных сообщений для тестирования

### 8.3 EventHandler

**Файл:** `src/common/event_handler.hpp`

**Функционал:**
- HTTP endpoints для тестирования:
  - `POST /api/v1/events/publish` - публикация тестового события
  - `GET /api/v1/events/consumed` - получение списка потребленных событий

---

## 9. Интеграция в существующие сервисы

### 9.1 User Service

**Изменения в `src/user/handlers.cpp`:**

```cpp
// В CreateUserHandler
auto created_user = database_.CreateUser(request);
producer_.PublishUserCreated(created_user);  // Новая строка
return created_user.ToJson();

// В UpdateUserHandler
auto updated_user = database_.UpdateUser(user_id, request);
producer_.PublishUserUpdated(updated_user);  // Новая строка
return updated_user.ToJson();
```

### 9.2 Folder Service

**Изменения в `src/folder/handlers.cpp`:**

```cpp
// В CreateFolderHandler
auto created_folder = database_.CreateFolder(user_id, request);
producer_.PublishFolderCreated(created_folder);  // Новая строка
return created_folder.ToJson();
```

### 9.3 Message Service

**Изменения в `src/message/handlers.cpp`:**

```cpp
// В CreateMessageHandler
auto created_message = database_.CreateMessage(folder_id, request);
producer_.PublishMessageCreated(created_message);  // Новая строка
return created_message.ToJson();

// В SendMessageHandler
auto message = database_.SendMessage(message_id);
producer_.PublishMessageSent(message);  // Новая строка
return message.ToJson();

// В MarkAsReadHandler
auto message = database_.MarkMessageAsRead(message_id);
producer_.PublishMessageRead(message_id, user_id);  // Новая строка
return message.ToJson();
```

---

## 10. Мониторинг и отладка

### 10.1 RabbitMQ Management UI

Доступен по адресу: `http://localhost:15672`

**Учетные данные:**
- Username: `guest`
- Password: `guest`

**Что можно мониторить:**
- Exchanges и их bindings
- Queues и количество сообщений
- Consumers и их статус
- Message rate (сообщений в секунду)

### 10.2 Логирование

Все события логируются в консоль:

```
[2024-05-03 11:10:00] [info] Publishing UserCreated event: user_id=507f1f77bcf86cd799439011
[2024-05-03 11:10:01] [info] Consumed UserCreated event: user_id=507f1f77bcf86cd799439011
```

---

## 11. Заключение

Event-Driven архитектура с RabbitMQ позволяет Email Service:

1. **Масштабироваться** - добавлять новые потребители без изменения producers
2. **Быть надежнее** - асинхронная обработка не блокирует основной поток
3. **Быть гибче** - легко добавлять новые функции через новые consumers
4. **Быть проще** - слабая связанность между компонентами

Применение CQRS паттерна позволяет:

1. **Оптимизировать** - read и write модели независимо
2. **Масштабировать** - read и write масштабируются отдельно
3. **Аудировать** - все изменения записываются как события

Выбор RabbitMQ обоснован его простотой, надежностью и встроенной поддержкой в userver.
