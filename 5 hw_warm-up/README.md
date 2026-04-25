# Отчёт по домашнему заданию №4: Проектирование и работа с MongoDB

**Студент группы М8О-106СВ-21:** Меркулов Фёдор Алексеевич  
**Вариант:** №9 — Электронная почта  

---

## Оглавление

1. [Что было сделано](#что-было-сделано)
2. [Схема базы данных](#схема-базы-данных)
3. [API Endpoints](#api-endpoints)
4. [Технические детали](#технические-детали)
5. [Тестирование](#тестирование)
6. [Инструкции по запуску](#инструкции-по-запуску)
7. [Структура проекта](#структура-проекта)

---

## Что было сделано

Это продолжение [домашнего задания №3](../3%20hw/README.md), в котором был реализован REST API для упрощённого почтового сервиса на C++ с использованием фреймворка Yandex uServer. В HW3 использовалась PostgreSQL — в рамках HW4 выполнена миграция на MongoDB 7.0, спроектирована документная модель данных, созданы индексы и настроена валидация схемы.

### Архитектура решения

Микросервисная архитектура с разделением по доменам:

- **User Service** (порт 8081) — управление пользователями и аутентификация
- **Folder Service** (порт 8082) — работа с почтовыми папками
- **Message Service** (порт 8083) — операции с письмами
- **Nginx** (порт 8080) — API Gateway, единая точка входа

---

## Схема базы данных

MongoDB использует три коллекции с JSON Schema валидацией:

### Коллекция `users`

```javascript
{
  _id: ObjectId,           // MongoDB auto-generated
  login: String,           // уникальный логин
  email: String,           // уникальный email
  firstName: String,
  lastName: String,
  passwordHash: String,
  createdAt: Date,
  updatedAt: Date
}
```

**Индексы:**
- `login` (unique)
- `email` (unique)
- `firstName + lastName`
- `createdAt`

### Коллекция `folders`

```javascript
{
  _id: ObjectId,           // MongoDB auto-generated
  userId: ObjectId,        // ссылка на users._id
  name: String,
  createdAt: Date,
  updatedAt: Date
}
```

**Индексы:**
- `userId + name` (unique)
- `userId`
- `createdAt`

### Коллекция `messages`

```javascript
{
  _id: ObjectId,           // MongoDB auto-generated
  folderId: ObjectId,      // ссылка на folders._id
  sender: {
    _id: ObjectId,         // ссылка на users._id
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

**Индексы:**
- `folderId + createdAt`
- `folderId`
- `sender._id`
- `createdAt`
- `recipientEmail`

---

### Тестовые данные

При инициализации БД автоматически создаются тестовые данные (см. [`data.js`](email-service/data.js)):

- **12 пользователей** — с логинами от `john_doe` до `julia_davis`
- **36 папок** — по 3 папки на каждого пользователя (Inbox, Sent, Drafts)
- **50 сообщений** — распределённых по папкам разных пользователей

Тестовые данные используются для интеграционного тестирования и демонстрации работы API.

---

## API Endpoints

Все 8 требуемых операций покрыты и работают с MongoDB:

### Аутентификация
- `POST /api/v1/users` — регистрация нового пользователя (коллекция `users`)
- `POST /api/v1/auth/login` — вход и получение JWT токена (коллекция `users`)

### Пользователи (требуют авторизации)
- `GET /api/v1/users/by-login?login=...` — поиск по логину (коллекция `users`)
- `GET /api/v1/users/search?mask=...` — поиск по маске имени/фамилии (коллекция `users`)

### Папки (требуют авторизации)
- `POST /api/v1/folders` — создание папки (коллекция `folders`)
- `GET /api/v1/folders` — список всех папок пользователя (коллекция `folders`)

### Письма (требуют авторизации)
- `POST /api/v1/folders/{folderId}/messages` — создание письма в папке (коллекция `messages`)
- `GET /api/v1/folders/{folderId}/messages` — список писем в папке (коллекция `messages`)
- `GET /api/v1/messages/{messageId}` — получение письма по ID (коллекция `messages`)

---

## Технические детали

**Стек технологий:**
- C++20 для основного кода
- Yandex uServer как веб-фреймворк
- **MongoDB 7.0** для хранения данных (миграция с PostgreSQL из HW3)
- MongoDB C++ Driver (mongocxx) для работы с БД
- JWT-CPP для работы с токенами
- Podman + Podman Compose для контейнеризации
- Nginx как reverse proxy
- Python + pytest для интеграционных тестов

**Особенности реализации:**
- JSON Schema валидация в MongoDB для всех коллекций
- Использование ObjectId для всех идентификаторов
- Встроенный объект sender в сообщениях для оптимизации чтения

### Подключение MongoDB к API

API подключается к MongoDB через URI, указываемый в конфигурации сервисов:

```yaml
# configs/static_config_mongo.yaml
mongo:
  uri: "mongodb://email_user:email_pass@email-mongodb:27017/email_db"
```

**Реализация подключения** ([`database.cpp:10-23`](email-service/src/common/database.cpp#L10)):
- Используется MongoDB C++ Driver (mongocxx)
- URI парсится для извлечения имени БД
- Создаётся клиент и получается доступ к коллекциям

**Коллекции используемые в API:**
| Коллекция | Описание | Файл с методами |
|-----------|----------|-----------------|
| `users` | Пользователи | [`database.cpp:120-200`](email-service/src/common/database.cpp#L120) |
| `folders` | Почтовые папки | [`database.cpp:269-330`](email-service/src/common/database.cpp#L269) |
| `messages` | Письма | [`database.cpp:353-450`](email-service/src/common/database.cpp#L353) |

---

## Реализация CRUD операций

В варианте 9 (Электронная почта) согласно заданию реализованы операции **Create** (создание) и **Read** (чтение) для всех 8 требуемых API эндпоинтов.

### Create — Создание документов

| Операция | MongoDB метод | Файл |
|----------|---------------|------|
| Создание пользователя | `insertOne()` | [`database.cpp:120`](email-service/src/common/database.cpp#L120) |
| Создание папки | `insertOne()` | [`database.cpp:269`](email-service/src/common/database.cpp#L269) |
| Создание сообщения | `insertOne()` | [`database.cpp:353`](email-service/src/common/database.cpp#L353) |

### Read — Чтение документов

| Операция | MongoDB метод | Операторы | Файл |
|----------|---------------|-----------|------|
| Поиск по логину | `findOne()` | — | [`database.cpp:186`](email-service/src/common/database.cpp#L186) |
| Поиск по маске имени | `find()` | `$or`, `$regex`, `$options: "i"` | [`database.cpp:219`](email-service/src/common/database.cpp#L219) |
| Список папок | `find()` | — | [`database.cpp:303`](email-service/src/common/database.cpp#L303) |
| Список сообщений | `find()` | — | [`database.cpp:414`](email-service/src/common/database.cpp#L414) |
| Поиск по ID | `findOne()` | `$oid` | [`database.cpp:433`](email-service/src/common/database.cpp#L433) |

### Использованные MongoDB операторы

- [`$or`](email-service/queries.js#L55) — логическое ИЛИ для поиска по firstName ИЛИ lastName
- [`$regex`](email-service/queries.js#L55) — регулярные выражения с флагом `$options: "i"` (регистронезависимый)
- [`$in`](email-service/queries.js#L126) — поиск по списку значений
- [`$gt`](email-service/queries.js#L129) — больше указанной даты
- [`$exists`](email-service/queries.js#L133) — проверка существования поля
- [`$oid`](email-service/src/common/database.cpp#L206) — работа с ObjectId

### Агрегации (опционально)

В проекте реализованы aggregation pipelines для аналитических запросов (см. [`queries.js:103-120`](email-service/queries.js#L103)):

#### Пример 1: Количество сообщений по папкам (топ-5)
```javascript
db.messages.aggregate([
  {$group: {_id: "$folderId", count: {$sum: 1}}},
  {$sort: {count: -1}},
  {$limit: 5},
  {$lookup: {from: "folders", localField: "_id", foreignField: "_id", as: "folder"}},
  {$unwind: "$folder"},
  {$project: {folderName: "$folder.name", count: 1, _id: 0}}
])
```
**Стадии**: `$group` → `$sort` → `$limit` → `$lookup` → `$unwind` → `$project`

#### Пример 2: Количество сообщений по отправителям (топ-5)
```javascript
db.messages.aggregate([
  {$group: {_id: "$sender.login", count: {$sum: 1}}},
  {$sort: {count: -1}},
  {$limit: 5}
])
```
**Стадии**: `$group` → `$sort` → `$limit`

### Примеры запросов

Полные примеры MongoDB запросов см. в файле [`queries.js`](email-service/queries.js):

```javascript
// Create
db.users.insertOne({login: "test_user", ...})
db.folders.insertOne({userId: john._id, name: "Test Folder", ...})
db.messages.insertOne({folderId: inbox._id, sender: {...}, ...})

// Read
db.users.findOne({login: "john_doe"})
db.users.find({ $or: [{firstName: {$regex: "John", $options: "i"}}, ...] })
db.folders.find({userId: john._id})
db.messages.find({folderId: inbox._id}).sort({createdAt: -1}).limit(3)
```

---

## Тестирование

Все 66 интеграционных тестов проходят успешно:

| Сервис | Тесты | Статус |
|--------|-------|--------|
| User Service | 23/23 | ✅ PASS |
| Folder Service | 16/16 | ✅ PASS |
| Message Service | 27/27 | ✅ PASS |
| **Итого** | **66/66** | **✅ PASS** |

---

## Инструкции по запуску

### Предварительные требования

- Podman (или Docker) и podman-compose
- Python 3.8+ для тестов

### Запуск основного окружения

```bash
cd 4 hw/email-service
podman-compose up -d --build
```

Ожидать 40-60 секунд для инициализации MongoDB и запуска сервисов.

### Доступ к API

- **Gateway (nginx):** http://localhost:8080
- **Swagger UI:** http://localhost:8080/swagger/
- **User Service:** http://localhost:8081
- **Folder Service:** http://localhost:8082
- **Message Service:** http://localhost:8083

### Выполнение запросов

Данные инициализации загружаются автоматически при запуске контейнера (`init-mongo.js`).

**Загрузка тестовых данных (data.js):**
```bash
cd 4 hw/email-service
podman exec -i email-mongodb mongosh -u email_user -p email_pass --authenticationDatabase email_db email_db < data.js
```

**CRUD операции (queries.js):**
```bash
cd 4 hw/email-service
podman exec -i email-mongodb mongosh -u email_user -p email_pass --authenticationDatabase email_db email_db < queries.js
```

**Валидация схемы (validation.js):**
```bash
cd 4 hw/email-service
podman exec -i email-mongodb mongosh -u email_user -p email_pass --authenticationDatabase email_db email_db < validation.js
```

**Перезагрузить тестовые данные (data.js):**
```bash
cd 4 hw/email-service
podman exec -i email-mongodb mongosh -u email_user -p email_pass --authenticationDatabase email_db email_db < data.js
```

### Запуск тестов

```bash
cd 4 hw/email-service
podman-compose -f docker-compose.test.yaml up -d
sleep 40

# Установить зависимости
podman exec email-user-service-test python3 -m pip install -q PyJWT pytest pytest-asyncio aiohttp pymongo yandex-taxi-testsuite

# Запустить все тесты
podman exec email-user-service-test python3 -m pytest /app/tests/ -v
```

### Остановка

```bash
# Основное окружение
podman-compose down -v

# Тестовое окружение
podman-compose -f docker-compose.test.yaml down -v
```

---

## Структура проекта

```
4 hw/
├── README.md                    # Этот файл
├── TASK_INFO.md                 # Описание задания
├── QUICK_START.md               # Быстрый старт
├── email-service/
│   ├── CMakeLists.txt           # Сборка C++
│   ├── Dockerfile               # Образ сервисов
│   ├── docker-compose.yaml      # Основное окружение
│   ├── docker-compose.test.yaml # Тестовое окружение
│   ├── nginx.conf               # Конфигурация nginx
│   ├── openapi.yaml             # OpenAPI спецификация
│   ├── init-mongo.js            # Инициализация MongoDB
│   ├── data.js                  # Тестовые данные
│   ├── queries.js               # Примеры запросов
│   ├── validation.js            # Валидация схемы
│   ├── schema_design.md         # Проектирование схемы
│   ├── configs/                 # Конфигурации сервисов
│   ├── src/
│   │   ├── auth/                # JWT аутентификация
│   │   ├── common/              # Общие компоненты (database, models)
│   │   ├── user/                # User Service
│   │   ├── folder/              # Folder Service
│   │   └── message/             # Message Service
│   ├── swagger/                 # Swagger UI
│   └── tests/                   # Интеграционные тесты
```
