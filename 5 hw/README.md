# Отчёт по домашнему заданию №5: Оптимизация производительности через кэширование и rate limiting

**Студент группы М8О-106СВ-21:** Меркулов Фёдор Алексеевич  
**Вариант:** №9 — Электронная почта

---

## Оглавление

1. [Что было сделано](#что-было-сделано)
2. [Оптимизация производительности](#оптимизация-производительности)
3. [Схема базы данных](#схема-базы-данных)
4. [API Endpoints](#api-endpoints)
5. [Технические детали](#технические-детали)
6. [Тестирование](#тестирование)
7. [Инструкции по запуску](#инструкции-по-запуску)
8. [Структура проекта](#структура-проекта)

**Дополнительные материалы:**
- [`performance_design.md`](performance_design.md) — детальное описание стратегии кеширования и rate limiting

---

## Что было сделано

Это продолжение [домашнего задания №4](../4%20hw/README.md), в котором была выполнена миграция на MongoDB 7.0. В рамках HW5 реализована оптимизация производительности через кэширование и rate limiting.

### Архитектура решения

Микросервисная архитектура с разделением по доменам:

- **User Service** (порт 8081) — управление пользователями и аутентификация
- **Folder Service** (порт 8082) — работа с почтовыми папками
- **Message Service** (порт 8083) — операции с письмами
- **Nginx** (порт 8080) — API Gateway, единая точка входа
- **Prometheus** (порт 9090) — сбор метрик
- **Grafana** (порт 3000) — визуализация метрик

---

## Оптимизация производительности

### Анализ производительности

**Hot Paths (часто выполняемые операции):**
- Поиск пользователя по логину (`GET /api/v1/users/login/{login}`) - выполняется при каждом входе
- Получение списка папок пользователя (`GET /api/v1/folders`) - выполняется при загрузке интерфейса
- Получение списка сообщений в папке (`GET /api/v1/folders/{id}/messages`) - выполняется при просмотре папки
- Поиск пользователей по имени (`GET /api/v1/users/search`) - выполняется при автодополнении

**Медленные операции:**
- Обращения к MongoDB (50-100ms latency)
- Сложные агрегации для поиска сообщений
- Операции с большими документами (сообщения с вложениями)

**Требования к производительности:**
- Время отклика для кэшируемых операций: < 5ms
- Время отклика для некэшируемых операций: < 100ms
- Пропускная способность: 1000+ RPS для кэшируемых операций
- Cache Hit Rate: > 80%

### Кэширование

Реализован in-memory кэш с использованием стратегии Cache-Aside (Lazy Loading):

**Кэшируемые данные:**
- Пользователи (по login)
- Папки (по userId)
- Сообщения (по folderId)

**Характеристики кэша:**
- TTL: 60 секунд по умолчанию
- LRU eviction при достижении лимита
- Thread-safe операции с mutex
- Автоматическая инвалидация при изменении данных

**Эффективность:**
- Cache Hit Rate: > 80% при повторных запросах
- Снижение нагрузки на MongoDB в 5-10 раз
- Уменьшение времени ответа с 50-100ms до 1-5ms

### Rate Limiting

Реализован алгоритм Token Bucket для ограничения запросов:

**Лимиты по endpoints:**
- User Registration: 5 запросов burst, 10 запросов в минуту
- Login: 5 запросов burst, 20 запросов в минуту
- Send Message: 10 запросов burst, 100 запросов в минуту

**Характеристики:**
- Per-IP и per-user tracking
- Автоматическая очистка старых bucket'ов
- HTTP заголовки с информацией о лимитах
- Статистика нарушений

**Эффективность:**
- Защита от DDoS атак
- Предотвращение спама
- Справедливое распределение ресурсов

### Мониторинг

**Метрики кэша:**
- `cache_hits_total` - количество попаданий в кэш
- `cache_misses_total` - количество промахов
- `cache_hit_rate` - процент попаданий
- `cache_evictions_total` - количество вытеснений
- `cache_size` - текущий размер кэша

**Метрики rate limiting:**
- `rate_limit_denied_requests_total` - количество отклоненных запросов
- `rate_limit_allowed_requests_total` - количество разрешенных запросов
- `rate_limit_active_buckets` - количество активных bucket'ов

**Доступ к метрикам:**
- HTTP endpoint: `/cache-rate-limiting-metrics`
- Prometheus: http://localhost:9090
- Grafana Dashboard: "Email Service - Cache & Rate Limiting"

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
cd 5 hw/email-service
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
cd 5 hw/email-service
podman exec -i email-mongodb mongosh -u email_user -p email_pass --authenticationDatabase email_db email_db < data.js
```

**CRUD операции (queries.js):**
```bash
cd 5 hw/email-service
podman exec -i email-mongodb mongosh -u email_user -p email_pass --authenticationDatabase email_db email_db < queries.js
```

**Валидация схемы (validation.js):**
```bash
cd 5 hw/email-service
podman exec -i email-mongodb mongosh -u email_user -p email_pass --authenticationDatabase email_db email_db < validation.js
```

**Перезагрузить тестовые данные (data.js):**
```bash
cd 5 hw/email-service
podman exec -i email-mongodb mongosh -u email_user -p email_pass --authenticationDatabase email_db email_db < data.js
```

### Тестирование кэширования и Rate Limiting

```bash
cd 5 hw/email-service/tests
./test_cache_rate_limiting.sh
```

Этот скрипт проверяет:
- **Rate Limiting** - ограничение запросов (5 запросов burst для регистрации)
- **Caching** - эффективность кэширования при повторных запросах

### Нагрузочное тестирование

```bash
cd 5 hw/email-service/tests
./load_test.sh
```

Комплексное нагрузочное тестирование с использованием Apache Bench (ab):
- User Registration, Login, Find User, List Folders, List Messages
- Результаты сохраняются в `./load_test_results/`
- Генерирует графики производительности

### Мониторинг

- **Grafana:** http://localhost:3000 (admin/admin)
- **Prometheus:** http://localhost:9090
- **Metrics:** http://localhost:8081/cache-rate-limiting-metrics

### Остановка

```bash
cd 5 hw/email-service
podman-compose down -v
```

---

## Структура проекта

```
5 hw/
├── README.md                    # Этот файл
├── TASK_INFO.md                 # Описание задания
├── QUICK_START.md               # Быстрый старт
├── email-service/
│   ├── CMakeLists.txt           # Сборка C++
│   ├── Dockerfile               # Образ сервисов
│   ├── docker-compose.yaml      # Основное окружение
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
