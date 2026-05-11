# 6 HW: Event-Driven Email Service — Быстрый старт

## Обзор

Реализация Event-Driven архитектуры email-сервиса (вариант №9 — Электронная почта) с использованием:
- **userver Framework** — C++ фреймворк для микросервисов
- **RabbitMQ** — брокер сообщений для публикации событий
- **MongoDB 7.0** — документная база данных
- **CQRS Pattern** — разделение команд (write) и запросов (read)
- **Docker Compose** — контейнеризация и оркестрация

## Архитектура

```
┌─────────────────────────────────────────────────────────────────┐
│                     Email Service (6 hw)                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────┐          │
│  │ User Service │  │Folder Service│  │Message Service│          │
│  │  (Port 8081) │  │  (Port 8082) │  │  (Port 8083)  │          │
│  └──────┬───────┘  └──────┬───────┘  └──────┬────────┘          │
│         │                 │                 │                   │
│         └─────────────────┼─────────────────┘                   │
│                           │                                     │
│                    Публикация событий                           │
│                    (EmailEventProducer)                         │
│                           │                                     │
│                           ▼                                     │
│                  ┌──────────────────┐                           │
│                  │   RabbitMQ       │                           │
│                  │ (Port 5672/15672)│                           │
│                  │ Exchange: email- │                           │
│                  │ events (fanout)  │                           │
│                  └──────────────────┘                           │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Nginx API Gateway (Port 8080 → внутренний 80)           │   │
│  │  Маршрутизация /api/v1/... → сервисы                     │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              MongoDB (Port 27017)                        │   │
│  │         (Write Database для всех сервисов)               │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Мониторинг: Prometheus (9090) + Grafana (3000)          │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Требования

- Docker и Docker Compose (v2) установлены
- 4 GB+ оперативной памяти
- 2 GB+ свободного места на диске
- `curl` и `jq` для тестирования

## Быстрый старт (5 минут)

### 1. Запустить все сервисы

```bash
cd "6 hw/email-service"
docker-compose up -d --build
```

Дождитесь готовности (30–60 секунд). Проверьте статус:
```bash
docker-compose ps
```

Все сервисы должны быть в состоянии `Up (healthy)`.

### 2. Проверить доступность

```bash
# Nginx Gateway (единая точка входа)
curl http://localhost:8080/ping
# Ожидаемый ответ: pong

# RabbitMQ Management UI
curl -s -u guest:guest http://localhost:15672/api/overview | jq .cluster_name
# Ожидаемый ответ: "rabbit@..."
```

### 3. Создать пользователя (через Nginx Gateway)

```bash
curl -s -X POST http://localhost:8080/api/v1/users \
  -H "Content-Type: application/json" \
  -d '{
    "login": "john_doe",
    "email": "john@example.com",
    "first_name": "John",
    "last_name": "Doe",
    "password": "secure_password_123"
  }' | jq .
```

Ожидаемый ответ:
```json
{
  "id": "...",
  "login": "john_doe",
  "email": "john@example.com",
  "first_name": "John",
  "last_name": "Doe"
}
```

> **Событие:** При создании пользователя `EmailEventProducer` автоматически публикует событие `UserCreated` в RabbitMQ exchange `email-events`.

### 4. Проверить событие в RabbitMQ

```bash
# Проверить, что очередь email-events-queue существует и содержит сообщения
curl -s -u guest:guest http://localhost:15672/api/queues/%2F \
  | jq '.[] | select(.name == "email-events-queue") | {name, messages, consumers}'
```

Или откройте в браузере: http://localhost:15672 (логин: `guest`, пароль: `guest`)
→ вкладка **Queues** → `email-events-queue` → **Get messages**

### 5. Получить JWT-токен

```bash
curl -s -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "login": "john_doe",
    "password": "secure_password_123"
  }' | jq .
```

Сохраните токен:
```bash
TOKEN=$(curl -s -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login": "john_doe", "password": "secure_password_123"}' | jq -r '.token')
```

### 6. Создать папку (требует авторизации)

```bash
curl -s -X POST http://localhost:8080/api/v1/folders \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{
    "name": "Inbox",
    "user_id": "USER_ID_FROM_STEP_3"
  }' | jq .
```

> **Событие:** Публикуется `FolderCreated` в RabbitMQ.

### 7. Создать сообщение в папке (требует авторизации)

```bash
FOLDER_ID="FOLDER_ID_FROM_STEP_6"

curl -s -X POST http://localhost:8080/api/v1/folders/$FOLDER_ID/messages \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{
    "sender_email": "john@example.com",
    "recipient_email": "recipient@example.com",
    "subject": "Hello World",
    "body": "This is a test message"
  }' | jq .
```

> **Событие:** Публикуется `MessageCreated` в RabbitMQ.

---

## API Endpoints (через Nginx Gateway — порт 8080)

Все запросы идут через `http://localhost:8080`.

### Аутентификация

| Метод | Endpoint | Описание |
|-------|----------|----------|
| POST | `/api/v1/users` | Создание нового пользователя |
| POST | `/api/v1/auth/login` | Вход и получение JWT-токена |

### Пользователи (требуют JWT)

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/api/v1/users/by-login?login=...` | Поиск пользователя по логину |
| GET | `/api/v1/users/search?mask=...` | Поиск по маске имени/фамилии |

### Папки (требуют JWT)

| Метод | Endpoint | Описание |
|-------|----------|----------|
| POST | `/api/v1/folders` | Создание новой папки |
| GET | `/api/v1/folders` | Список всех папок пользователя |

### Письма (требуют JWT)

| Метод | Endpoint | Описание |
|-------|----------|----------|
| POST | `/api/v1/folders/{folderId}/messages` | Создание письма в папке |
| GET | `/api/v1/folders/{folderId}/messages` | Список писем в папке |
| GET | `/api/v1/messages/{messageId}` | Получение письма по ID |

### Служебные

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/ping` | Health check |
| GET | `/health` | Health check |
| GET | `/swagger/` | Swagger UI |
| GET | `/openapi.yaml` | OpenAPI спецификация |

---

## Типы событий

При выполнении write-операций `EmailEventProducer` публикует события в RabbitMQ:

| Событие | Производитель | Триггер (API) | Реально вызывается |
|---------|---------------|---------------|-------------------|
| `UserCreated` | User Service | `POST /api/v1/users` | ✅ Да |
| `FolderCreated` | Folder Service | `POST /api/v1/folders` | ✅ Да |
| `MessageCreated` | Message Service | `POST /api/v1/folders/{id}/messages` | ✅ Да |
| `UserUpdated` | User Service | (обновление профиля) | ⚠️ Задекларирован в producer |
| `MessageSent` | Message Service | (отправка сообщения) | ⚠️ Задекларирован в producer |
| `MessageRead` | Message Service | (прочтение сообщения) | ⚠️ Задекларирован в producer |

> **Примечание:** События `UserUpdated`, `MessageSent`, `MessageRead` реализованы в коде producer, но соответствующие API-хендлеры пока не вызывают их. Три основных события (`UserCreated`, `FolderCreated`, `MessageCreated`) полностью интегрированы.

---

## Мониторинг

| Сервис | URL | Логин/Пароль |
|--------|-----|-------------|
| RabbitMQ Management UI | http://localhost:15672 | guest / guest |
| Grafana | http://localhost:3000 | admin / admin |
| Prometheus | http://localhost:9090 | — |
| Swagger UI | http://localhost:8080/swagger/ | — |
| Метрики кэша | http://localhost:8081/cache-rate-limiting-metrics | — |

---

## Автоматическое тестирование Event-Driven

```bash
cd "6 hw"
chmod +x test_event_driven.sh
./test_event_driven.sh
```

Скрипт автоматически:
1. Проверяет доступность всех сервисов
2. Создаёт пользователя → проверяет событие `UserCreated` в RabbitMQ
3. Получает JWT-токен
4. Создаёт папку → проверяет событие `FolderCreated`
5. Создаёт сообщение → проверяет событие `MessageCreated`
6. Проверяет RabbitMQ Management API
7. Проверяет CQRS (read-модель)

---

## Логи сервисов

```bash
cd "6 hw/email-service"

# Все логи
docker-compose logs -f

# Конкретный сервис
docker-compose logs -f user-service
docker-compose logs -f folder-service
docker-compose logs -f message-service
docker-compose logs -f rabbitmq
```

---

## Остановка

```bash
cd "6 hw/email-service"

# Остановить (сохранить данные)
docker-compose down

# Остановить и удалить все данные
docker-compose down -v
```

---

## Документация

| Файл | Описание |
|------|----------|
| [`README.md`](README.md) | Полный отчёт по ДЗ №6 |
| [`event_driven_design.md`](event_driven_design.md) | Проектирование Event-Driven архитектуры |
| [`event_catalog.md`](event_catalog.md) | Каталог событий |
| [`performance_design.md`](performance_design.md) | Стратегия кэширования и rate limiting |
| [`test_event_driven.sh`](test_event_driven.sh) | Автоматический тест Event-Driven pipeline |
