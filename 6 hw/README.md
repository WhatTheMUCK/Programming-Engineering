# Отчёт по домашнему заданию №6: Event-Driven архитектура

**Студент группы М8О-106СВ-21:** Меркулов Фёдор Алексеевич  
**Вариант:** №9 — Электронная почта

---

## Оглавление

1. [Что было сделано](#что-было-сделано)
2. [Архитектура системы](#архитектура-системы)
3. [Анализ событий и команд](#анализ-событий-и-команд)
4. [Брокер сообщений (RabbitMQ)](#брокер-сообщений-rabbitmq)
5. [Применение CQRS](#применение-cqrs)
6. [Реализация Producer и Consumer](#реализация-producer-и-consumer)
7. [Каталог событий](#каталог-событий)
8. [API Endpoints](#api-endpoints)
9. [Схема базы данных](#схема-базы-данных)
10. [Тестирование](#тестирование)
11. [Инструкции по запуску](#инструкции-по-запуску)
12. [Мониторинг](#мониторинг)
13. [Структура проекта](#структура-проекта)

**Дополнительные материалы:**
- [`event_driven_design.md`](event_driven_design.md) — проектирование Event-Driven архитектуры
- [`event_catalog.md`](event_catalog.md) — каталог событий с описанием
- [`performance_design.md`](performance_design.md) — стратегия кэширования и rate limiting
- [`QUICK_START.md`](QUICK_START.md) — быстрый старт

---

## Что было сделано

Это продолжение [домашнего задания №5](../5%20hw/), в котором была реализована оптимизация производительности через кэширование и rate limiting. В рамках HW6 реализована **Event-Driven архитектура** с использованием RabbitMQ и паттерна CQRS.

### Новое в HW6 (поверх HW5)

| Компонент | Описание | Файл |
|-----------|----------|------|
| **RabbitMQ** | Брокер сообщений в docker-compose | [`docker-compose.yaml`](email-service/docker-compose.yaml) (строки 2–19) |
| **EmailEventProducer** | Публикация событий в RabbitMQ | [`rabbitmq_producer.hpp`](email-service/src/common/rabbitmq_producer.hpp) |
| **EmailEventConsumer** | Потребление событий из RabbitMQ | [`rabbitmq_consumer.hpp`](email-service/src/common/rabbitmq_consumer.hpp) |
| **EventHandler** | HTTP-эндпоинт для тестирования событий | [`event_handler.hpp`](email-service/src/common/event_handler.hpp) |
| **ReadModelCache** | Кэш read-модели для CQRS | [`read_model_cache.hpp`](email-service/src/common/read_model_cache.hpp) |
| **ReadModelHandler** | HTTP-хендлер для read-модели | [`read_model_handler.cpp`](email-service/src/common/read_model_handler.cpp) |
| **ReadModelSynchronizer** | Синхронизация read-модели через события | [`read_model_synchronizer.hpp`](email-service/src/common/read_model_synchronizer.hpp) |
| **Документация** | Event-Driven дизайн и каталог событий | [`event_driven_design.md`](event_driven_design.md), [`event_catalog.md`](event_catalog.md) |
| **Тестовый скрипт** | End-to-end тест Event-Driven pipeline | [`test_event_driven.sh`](test_event_driven.sh) |

---

## Архитектура системы

```
┌─────────────────────────────────────────────────────────────────────┐
│                     Email Service (6 hw)                            │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  Nginx API Gateway (host port 8080 → container port 80)      │   │
│  │  Маршрутизация /api/v1/... → микросервисы                    │   │
│  └──────────────────────────────────────────────────────────────┘   │
│         │                    │                    │                 │
│         ▼                    ▼                    ▼                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐           │
│  │ User Service │  │Folder Service│  │ Message Service  │           │
│  │  Port 8081   │  │  Port 8082   │  │   Port 8083      │           │
│  │  Monitor:9091│  │  Monitor:9092│  │   Monitor:9093   │           │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────────┘           │
│         │                 │                  │                      │
│         │  PublishUserCreated  PublishFolderCreated  PublishMessage │
│         │                 │                  │      Created         │
│         └─────────────────┼──────────────────┘                      │
│                           │                                         │
│                           ▼                                         │
│                  ┌──────────────────┐                               │
│                  │   RabbitMQ       │                               │
│                  │ AMQP: 5672       │                               │
│                  │ UI:   15672      │                               │
│                  │                  │                               │
│                  │ Exchange:        │                               │
│                  │  email-events    │                               │
│                  │  (type: fanout)  │                               │
│                  │                  │                               │
│                  │ Queue:           │                               │
│                  │  email-events-   │                               │
│                  │  queue           │                               │
│                  └──────────────────┘                               │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │              MongoDB 7.0 (Port 27017)                        │   │
│  │         Write Database для всех сервисов                     │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  Мониторинг                                                  │   │
│  │  Prometheus (9090) │ Grafana (3000) │ Node Exporter (9100)   │   │
│  │  MongoDB Exporter (9216) │ Nginx Exporter (9113)             │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Стек технологий

- **C++20** — основной код сервисов
- **Yandex uServer** — веб-фреймворк для микросервисов
- **MongoDB 7.0** — документная база данных
- **RabbitMQ 3.12** — брокер сообщений (с Management UI)
- **Nginx** — API Gateway (reverse proxy)
- **Docker Compose v2** — контейнеризация
- **Prometheus + Grafana** — мониторинг
- **JWT** — аутентификация
- **Python + pytest** — интеграционные тесты

---

## Анализ событий и команд

### Команды (Write-операции) → генерируют события

| Команда | HTTP Endpoint | Событие | Статус интеграции |
|---------|---------------|---------|-------------------|
| CreateUser | `POST /api/v1/users` | `UserCreated` | ✅ Вызывается в хендлере |
| CreateFolder | `POST /api/v1/folders` | `FolderCreated` | ✅ Вызывается в хендлере |
| CreateMessage | `POST /api/v1/folders/{id}/messages` | `MessageCreated` | ✅ Вызывается в хендлере |
| UpdateUser | (обновление профиля) | `UserUpdated` | ⚠️ Метод в producer есть, хендлер не вызывает |
| SendMessage | (отправка сообщения) | `MessageSent` | ⚠️ Метод в producer есть, хендлер не вызывает |
| MarkAsRead | (прочтение сообщения) | `MessageRead` | ⚠️ Метод в producer есть, хендлер не вызывает |

### Запросы (Read-операции) → НЕ генерируют события

| Запрос | HTTP Endpoint |
|--------|---------------|
| FindUserByLogin | `GET /api/v1/users/by-login?login=...` |
| SearchUsersByName | `GET /api/v1/users/search?mask=...` |
| ListFolders | `GET /api/v1/folders` |
| ListMessages | `GET /api/v1/folders/{folderId}/messages` |
| GetMessage | `GET /api/v1/messages/{messageId}` |

### Потребители событий

Каждое событие может быть обработано следующими потребителями:

| Событие | Потребители |
|---------|-------------|
| `UserCreated` | Notification Service, Analytics Service, Audit Service |
| `UserUpdated` | Audit Service, Analytics Service |
| `FolderCreated` | Analytics Service, Audit Service |
| `MessageCreated` | Search Service, Analytics Service, Audit Service |
| `MessageSent` | Notification Service, Analytics Service, Audit Service |
| `MessageRead` | Analytics Service, Audit Service |

---

## Брокер сообщений (RabbitMQ)

### Обоснование выбора RabbitMQ

1. **Встроенная поддержка в userver** — `userver::rabbitmq` модуль
2. **Надёжная доставка** — at-least-once через `PublishReliable` + manual ACK
3. **Management UI** — встроенный веб-интерфейс для мониторинга
4. **Простота** — достаточна для email-сервиса (не нужен Kafka-уровень throughput)

### Топология

```
Exchange: email-events
├── Type: fanout
├── Durable: true
└── Auto-delete: false

Queue: email-events-queue
├── Durable: true
├── Auto-delete: false
└── Binding: email-events → email-events-queue (routing key: email-routing-key)
```

### Гарантии доставки: at-least-once

**Как это работает:**
1. Producer вызывает `client_->PublishReliable(...)` — RabbitMQ подтверждает получение
2. Сообщение сохраняется в durable queue
3. Consumer получает сообщение и обрабатывает его
4. При успехе — ACK отправляется автоматически
5. При ошибке — `throw` в `Process()` → NACK → сообщение возвращается в очередь

**Код producer** ([`rabbitmq_producer.hpp:200`](email-service/src/common/rabbitmq_producer.hpp)):
```cpp
client_->PublishReliable(
    exchange_,
    routing_key_,
    message,
    urabbitmq::MessageType::kTransient,
    engine::Deadline::FromDuration(std::chrono::seconds{2})
);
```

**Код consumer** ([`rabbitmq_consumer.hpp:45`](email-service/src/common/rabbitmq_consumer.hpp)):
```cpp
void Process(std::string message) override {
    try {
        auto json = userver::formats::json::FromString(message);
        // ... обработка ...
        ProcessEvent(json);
    } catch (const std::exception& e) {
        throw;  // Возврат сообщения в очередь для retry
    }
}
```

### Формат сообщений

Все события используют единый JSON-формат:

```json
{
  "event_id": "uuid-v4",
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

### Конфигурация RabbitMQ в docker-compose

```yaml
rabbitmq:
  image: rabbitmq:3.12-management-alpine
  environment:
    RABBITMQ_DEFAULT_USER: guest
    RABBITMQ_DEFAULT_PASS: guest
    RABBITMQ_DEFAULT_VHOST: /
  ports:
    - "5672:5672"      # AMQP
    - "15672:15672"    # Management UI
  healthcheck:
    test: ["CMD", "rabbitmq-diagnostics", "ping"]
```

Каждый сервис подключается к RabbitMQ через `SECDIST_CONFIG`:
```json
{
  "rabbitmq_settings": {
    "email-rabbit-alias": {
      "hosts": ["rabbitmq"],
      "port": 5672,
      "login": "guest",
      "password": "guest",
      "vhost": "/"
    }
  }
}
```

И регистрирует producer в конфигурации:
```yaml
my-rabbit:
    secdist_alias: email-rabbit-alias
    min_pool_size: 5
    max_pool_size: 10

email-event-producer:
    rabbit_name: my-rabbit
```

---

## Применение CQRS

### Разделение на Commands и Queries

**Commands (Write)** — изменяют состояние и публикуют события:
```
CreateUser   → MongoDB insert → PublishUserCreated → RabbitMQ
CreateFolder → MongoDB insert → PublishFolderCreated → RabbitMQ
CreateMessage → MongoDB insert → PublishMessageCreated → RabbitMQ
```

**Queries (Read)** — только читают данные:
```
FindUserByLogin    → MongoDB find (+ in-memory cache)
SearchUsersByName  → MongoDB find (+ in-memory cache)
ListFolders        → MongoDB find (+ in-memory cache)
ListMessages       → MongoDB find
GetMessageById     → MongoDB findOne
```

### Синхронизация Read и Write моделей

```
Write Side                    Read Side
┌──────────────┐              ┌──────────────────┐
│ API Handler  │              │ In-Memory Cache  │
│ (Command)    │              │ (CacheComponent) │
│              │              │                  │
│ MongoDB      │──events──→   │ Оптимизированные │
│ (Write DB)   │  RabbitMQ    │ структуры для    │
│              │              │ быстрого чтения  │
└──────────────┘              └──────────────────┘
```

### Компоненты CQRS в коде

| Компонент | Файл | Назначение |
|-----------|------|------------|
| ReadModelCache | [`read_model_cache.hpp`](email-service/src/common/read_model_cache.hpp) | In-memory кэш read-модели |
| ReadModelHandler | [`read_model_handler.cpp`](email-service/src/common/read_model_handler.cpp) | HTTP-эндпоинты для read-модели (`/v1/read-models/...`) |
| ReadModelSynchronizer | [`read_model_synchronizer.hpp`](email-service/src/common/read_model_synchronizer.hpp) | Синхронизация read-модели через события |
| CacheComponent | [`cache_component.hpp`](email-service/src/common/cache_component.hpp) | Кэш для query-операций (Cache-Aside) |

> **Примечание:** `ReadModelHandler` и `ReadModelSynchronizer` реализованы в коде, но не зарегистрированы в YAML-конфигурациях сервисов. Текущая read-модель работает через `CacheComponent` (Cache-Aside pattern), который зарегистрирован и активен.

---

## Реализация Producer и Consumer

### EmailEventProducer

**Файл:** [`email-service/src/common/rabbitmq_producer.hpp`](email-service/src/common/rabbitmq_producer.hpp)

Компонент userver, зарегистрированный во всех трёх сервисах. При старте:
1. Создаёт exchange `email-events` (fanout)
2. Создаёт queue `email-events-queue`
3. Привязывает queue к exchange

Публичные методы:
- `PublishUserCreated(const User&)` — **вызывается** в [`user/handlers.cpp:92`](email-service/src/user/handlers.cpp)
- `PublishUserUpdated(const User&)` — задекларирован
- `PublishFolderCreated(const Folder&)` — **вызывается** в [`folder/handlers.cpp:66`](email-service/src/folder/handlers.cpp)
- `PublishMessageCreated(const Message&)` — **вызывается** в [`message/handlers.cpp:107`](email-service/src/message/handlers.cpp)
- `PublishMessageSent(const Message&)` — задекларирован
- `PublishMessageRead(...)` — задекларирован

### EmailEventConsumer

**Файл:** [`email-service/src/common/rabbitmq_consumer.hpp`](email-service/src/common/rabbitmq_consumer.hpp)

Наследуется от `userver::urabbitmq::ConsumerComponentBase`. Реализует:
- `Process(std::string message)` — парсит JSON, диспетчеризует по `event_type`
- 6 обработчиков: `ProcessUserCreated`, `ProcessUserUpdated`, `ProcessFolderCreated`, `ProcessMessageCreated`, `ProcessMessageSent`, `ProcessMessageRead`
- `GetConsumedMessages()` — для тестирования
- Thread-safe хранилище обработанных сообщений

> **Примечание:** Consumer реализован в коде, но не зарегистрирован в YAML-конфигурациях. События публикуются в RabbitMQ и доступны для потребления внешними сервисами или через RabbitMQ Management UI.

### EventHandler (тестовый)

**Файл:** [`email-service/src/common/event_handler.hpp`](email-service/src/common/event_handler.hpp)

HTTP-хендлер для ручного тестирования событий:
- `POST` — публикация тестового события (поддерживает все 6 типов)
- `GET` — получение списка потреблённых событий
- `DELETE` — очистка списка потреблённых событий

### Интеграция в CMakeLists.txt

Все три сервиса линкуются с `userver::rabbitmq` ([`CMakeLists.txt`](email-service/CMakeLists.txt)):
```cmake
target_link_libraries(email-service-user PUBLIC
    userver::mongo
    userver::rabbitmq
    ${MONGOCXX_LIBRARIES}
)
```

---

## Каталог событий

Полный каталог — в файле [`event_catalog.md`](event_catalog.md). Краткая сводка:

| Событие | Производитель | Потребители | Гарантия | Payload |
|---------|---------------|-------------|----------|---------|
| `UserCreated` | User Service | Notification, Analytics, Audit | at-least-once | user_id, login, email, first_name, last_name, created_at |
| `UserUpdated` | User Service | Audit, Analytics | at-least-once | user_id, login, email, first_name, last_name, updated_at |
| `FolderCreated` | Folder Service | Analytics, Audit | at-least-once | folder_id, user_id, name, created_at |
| `MessageCreated` | Message Service | Search, Analytics, Audit | at-least-once | message_id, folder_id, sender_id, recipient_email, subject, body, is_sent, created_at |
| `MessageSent` | Message Service | Notification, Analytics, Audit | at-least-once | message_id, folder_id, sender_id, recipient_email, subject, sent_at |
| `MessageRead` | Message Service | Analytics, Audit | at-least-once | message_id, folder_id, user_id, read_at |

---

## API Endpoints

Все запросы идут через **Nginx API Gateway** на `http://localhost:8080`.

### Аутентификация

| Метод | Endpoint | Описание | Событие |
|-------|----------|----------|---------|
| `POST` | `/api/v1/users` | Создание нового пользователя | `UserCreated` ✅ |
| `POST` | `/api/v1/auth/login` | Вход и получение JWT-токена | — |

### Пользователи (требуют JWT)

| Метод | Endpoint | Описание | Событие |
|-------|----------|----------|---------|
| `GET` | `/api/v1/users/by-login?login=...` | Поиск по логину | — |
| `GET` | `/api/v1/users/search?mask=...` | Поиск по маске имени/фамилии | — |

### Папки (требуют JWT)

| Метод | Endpoint | Описание | Событие |
|-------|----------|----------|---------|
| `POST` | `/api/v1/folders` | Создание папки | `FolderCreated` ✅ |
| `GET` | `/api/v1/folders` | Список всех папок | — |

### Письма (требуют JWT)

| Метод | Endpoint | Описание | Событие |
|-------|----------|----------|---------|
| `POST` | `/api/v1/folders/{folderId}/messages` | Создание письма | `MessageCreated` ✅ |
| `GET` | `/api/v1/folders/{folderId}/messages` | Список писем в папке | — |
| `GET` | `/api/v1/messages/{messageId}` | Получение письма по ID | — |

### Служебные (через Nginx)

| Метод | Endpoint | Описание |
|-------|----------|----------|
| `GET` | `/ping` | Health check (возвращает `pong`) |
| `GET` | `/health` | Health check (возвращает `healthy`) |
| `GET` | `/swagger/` | Swagger UI |
| `GET` | `/openapi.yaml` | OpenAPI спецификация |

### Прямой доступ к сервисам (без Nginx)

| Сервис | URL | Метрики |
|--------|-----|---------|
| User Service | `http://localhost:8081` | `http://localhost:8081/cache-rate-limiting-metrics` |
| Folder Service | `http://localhost:8082` | — |
| Message Service | `http://localhost:8083` | — |

---

## Схема базы данных

MongoDB использует три коллекции с JSON Schema валидацией:

### Коллекция `users`

```javascript
{
  _id: ObjectId,
  login: String,        // уникальный
  email: String,        // уникальный
  firstName: String,
  lastName: String,
  passwordHash: String,
  createdAt: Date,
  updatedAt: Date
}
```

**Индексы:** `login` (unique), `email` (unique), `firstName + lastName`, `createdAt`

### Коллекция `folders`

```javascript
{
  _id: ObjectId,
  userId: ObjectId,     // ссылка на users._id
  name: String,
  createdAt: Date,
  updatedAt: Date
}
```

**Индексы:** `userId + name` (unique), `userId`, `createdAt`

### Коллекция `messages`

```javascript
{
  _id: ObjectId,
  folderId: ObjectId,   // ссылка на folders._id
  sender: {
    _id: ObjectId,
    login: String,
    email: String,
    firstName: String,
    lastName: String
  },
  recipientEmail: String,
  subject: String,
  body: String,
  isSent: Boolean,
  createdAt: Date,
  updatedAt: Date
}
```

**Индексы:** `folderId + createdAt`, `folderId`, `sender._id`, `createdAt`, `recipientEmail`

### Тестовые данные

При инициализации БД автоматически создаются тестовые данные (см. [`data.js`](email-service/data.js)):
- **12 пользователей** — с логинами от `john_doe` до `julia_davis`
- **36 папок** — по 3 папки на каждого пользователя (Inbox, Sent, Drafts)
- **50 сообщений** — распределённых по папкам разных пользователей

---

## Тестирование

### Автоматический тест Event-Driven pipeline

```bash
cd "6 hw"
chmod +x test_event_driven.sh
./test_event_driven.sh
```

Скрипт [`test_event_driven.sh`](test_event_driven.sh) выполняет 6 тестов:

| Тест | Что проверяет |
|------|---------------|
| 1 | Создание пользователя → событие `UserCreated` в RabbitMQ |
| 2 | Получение JWT-токена |
| 3 | Создание папки → событие `FolderCreated` в RabbitMQ |
| 4 | Создание сообщения → событие `MessageCreated` в RabbitMQ |
| 5 | RabbitMQ Management API — наличие очереди и consumers |
| 6 | CQRS — read-модель синхронизирована с write-моделью |

### Интеграционные тесты (pytest)

```bash
cd "6 hw/email-service/tests"
pip install -r requirements.txt
pytest -v
```

| Сервис | Файл | Тесты |
|--------|------|-------|
| User Service | [`test_user_service.py`](email-service/tests/test_user_service.py) | 23 |
| Folder Service | [`test_folder_service.py`](email-service/tests/test_folder_service.py) | 16 |
| Message Service | [`test_message_service.py`](email-service/tests/test_message_service.py) | 27 |

### Ручная проверка событий через RabbitMQ UI

1. Откройте http://localhost:15672 (guest/guest)
2. Перейдите на вкладку **Queues** → `email-events-queue`
3. Нажмите **Get messages** (Ack mode: Nack message requeue true)
4. Увидите JSON-события с полями `event_id`, `event_type`, `timestamp`, `version`, `data`

---

## Инструкции по запуску

### Предварительные требования

- Docker и Docker Compose (v2)
- Python 3.8+ для тестов
- `curl` и `jq` для ручного тестирования

### Запуск

```bash
cd "6 hw/email-service"
docker-compose up -d --build
```

Ожидать 40–60 секунд для инициализации MongoDB, RabbitMQ и запуска сервисов.

### Проверка готовности

```bash
# Все контейнеры должны быть healthy
docker-compose ps

# Nginx Gateway
curl http://localhost:8080/ping

# RabbitMQ
curl -s -u guest:guest http://localhost:15672/api/overview | jq .cluster_name
```

### Примеры запросов

```bash
# 1. Создать пользователя (публикует UserCreated)
curl -s -X POST http://localhost:8080/api/v1/users \
  -H "Content-Type: application/json" \
  -d '{
    "login": "test_user",
    "email": "test@example.com",
    "first_name": "Test",
    "last_name": "User",
    "password": "password123"
  }' | jq .

# 2. Получить JWT-токен
TOKEN=$(curl -s -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login": "test_user", "password": "password123"}' | jq -r '.token')

# 3. Найти пользователя по логину
curl -s "http://localhost:8080/api/v1/users/by-login?login=test_user" \
  -H "Authorization: Bearer $TOKEN" | jq .

# 4. Поиск по маске имени
curl -s "http://localhost:8080/api/v1/users/search?mask=Test" \
  -H "Authorization: Bearer $TOKEN" | jq .

# 5. Создать папку (публикует FolderCreated)
curl -s -X POST http://localhost:8080/api/v1/folders \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{"name": "Inbox", "user_id": "USER_ID"}' | jq .

# 6. Список папок
curl -s http://localhost:8080/api/v1/folders \
  -H "Authorization: Bearer $TOKEN" | jq .

# 7. Проверить события в RabbitMQ
curl -s -u guest:guest http://localhost:15672/api/queues/%2F \
  | jq '.[] | select(.name == "email-events-queue") | {name, messages}'
```

### Загрузка тестовых данных

```bash
cd "6 hw/email-service"
docker exec -i email-mongodb mongosh -u email_user -p email_pass \
  --authenticationDatabase email_db email_db < data.js
```

### Остановка

```bash
cd "6 hw/email-service"

# Остановить (сохранить данные)
docker-compose down

# Остановить и удалить все данные
docker-compose down -v
```

---

## Мониторинг

| Сервис | URL | Логин/Пароль |
|--------|-----|-------------|
| **RabbitMQ Management UI** | http://localhost:15672 | guest / guest |
| **Grafana** | http://localhost:3000 | admin / admin |
| **Prometheus** | http://localhost:9090 | — |
| **Swagger UI** | http://localhost:8080/swagger/ | — |
| **Метрики кэша** | http://localhost:8081/cache-rate-limiting-metrics | — |

### RabbitMQ Management UI

В RabbitMQ UI можно:
- Просмотреть exchange `email-events` (вкладка **Exchanges**)
- Просмотреть очередь `email-events-queue` (вкладка **Queues**)
- Посмотреть содержимое сообщений (**Get messages**)
- Мониторить скорость публикации/потребления

---

## Структура проекта

```
6 hw/
├── README.md                          # Этот файл (отчёт по ДЗ №6)
├── QUICK_START.md                     # Быстрый старт
├── TASK_INFO.md                       # Описание задания
├── event_driven_design.md             # Проектирование Event-Driven архитектуры
├── event_catalog.md                   # Каталог событий
├── performance_design.md              # Стратегия кэширования и rate limiting (HW5)
├── test_event_driven.sh               # Тестовый скрипт Event-Driven pipeline
├── email-service/
│   ├── CMakeLists.txt                 # Сборка C++ (линкует userver::rabbitmq)
│   ├── Dockerfile                     # Образ сервисов
│   ├── docker-compose.yaml            # Все контейнеры (RabbitMQ, MongoDB, сервисы, мониторинг)
│   ├── nginx.conf                     # API Gateway (/api/v1/... → сервисы)
│   ├── openapi.yaml                   # OpenAPI спецификация
│   ├── init-mongo.js                  # Инициализация MongoDB
│   ├── data.js                        # Тестовые данные
│   ├── queries.js                     # Примеры MongoDB запросов
│   ├── validation.js                  # Валидация схемы
│   ├── schema_design.md               # Проектирование схемы БД
│   ├── configs/
│   │   ├── static_config_user.yaml    # Конфиг User Service (порт 8081)
│   │   ├── static_config_folder.yaml  # Конфиг Folder Service (порт 8082)
│   │   └── static_config_message.yaml # Конфиг Message Service (порт 8083)
│   ├── src/
│   │   ├── auth/                      # JWT аутентификация
│   │   ├── common/
│   │   │   ├── rabbitmq_producer.hpp  # ★ Event Producer (RabbitMQ)
│   │   │   ├── rabbitmq_consumer.hpp  # ★ Event Consumer (RabbitMQ)
│   │   │   ├── event_handler.hpp      # ★ HTTP-хендлер для тестирования событий
│   │   │   ├── read_model_cache.*     # ★ CQRS Read Model Cache
│   │   │   ├── read_model_handler.*   # ★ CQRS Read Model HTTP Handler
│   │   │   ├── read_model_synchronizer.hpp # ★ Read Model Synchronizer
│   │   │   ├── database.*             # MongoDB операции
│   │   │   ├── models.*               # Модели данных
│   │   │   ├── cache_component.hpp    # In-memory кэш (Cache-Aside)
│   │   │   ├── rate_limiter.hpp       # Rate Limiting (Token Bucket)
│   │   │   ├── metrics_handler.*      # Метрики
│   │   │   └── mongo_component.hpp    # MongoDB компонент
│   │   ├── user/                      # User Service (handlers + main)
│   │   ├── folder/                    # Folder Service (handlers + main)
│   │   └── message/                   # Message Service (handlers + main)
│   ├── monitoring/
│   │   ├── grafana/                   # Dashboards и provisioning
│   │   └── prometheus/                # Конфигурация и алерты
│   ├── swagger/                       # Swagger UI
│   └── tests/                         # Интеграционные тесты (pytest)
```

Файлы, помеченные ★, — новые в HW6 (Event-Driven архитектура).
