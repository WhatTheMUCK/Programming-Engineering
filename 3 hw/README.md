# Отчёт по домашнему заданию №3: Проектирование и оптимизация реляционной базы данных

**Студент группы М8О-106СВ-21:** Меркулов Фёдор Алексеевич  
**Вариант:** №9 — Электронная почта  

---

## Оглавление

1. [Что было сделано](#что-было-сделано)
2. [Схема базы данных](#схема-базы-данных)
3. [Индексы](#индексы)
4. [Оптимизация запросов](#оптимизация-запросов)
5. [Партиционирование](#партиционирование)
6. [Подключение API к базе данных](#подключение-api-к-базе-данных)
7. [Тестирование](#тестирование)
8. [Инструкции по запуску](#инструкции-по-запуску)
9. [Структура проекта](#структура-проекта)
10. [Чеклист выполнения задания](#чеклист-выполнения-задания)

---

## Что было сделано

Это продолжение [домашнего задания №2](../2%20hw/README.md), в котором был реализован REST API для упрощённого почтового сервиса на C++ с использованием фреймворка Yandex uServer. В HW2 использовалась SQLite — в рамках HW3 выполнена миграция на PostgreSQL 16, спроектирована реляционная схема БД, созданы индексы и проведена оптимизация запросов.

> **Примечание о контейнеризации:** В процессе выполнения HW3 была проведена миграция с Docker на Podman. Причина — на рабочем оборудовании (которое преимущественно используется для учёбы) поступило требование удалить Docker с устройства. Все команды в документации приведены для `podman-compose`, но полностью совместимы с `docker-compose` (достаточно заменить `podman-compose` → `docker-compose`).

### Архитектура решения

Вместо монолита сделал разделение по доменам:

- **User Service** (порт 8081) — управление пользователями и аутентификация
- **Folder Service** (порт 8082) — работа с почтовыми папками
- **Message Service** (порт 8083) — операции с письмами
- **Nginx** (порт 8080) — API Gateway, единая точка входа

Такой подход даёт гибкость: каждый сервис можно масштабировать отдельно, а если один упадёт, остальные продолжат работать.

### Реализованные API endpoints

Все 8 требуемых операций покрыты:

**Пользователи:**
- `POST /api/v1/users` — регистрация нового пользователя
- `POST /api/v1/auth/login` — вход и получение JWT токена
- `GET /api/v1/users/by-login?login=...` — поиск по логину (требует авторизации)
- `GET /api/v1/users/search?mask=...` — поиск по маске имени/фамилии (требует авторизации)

**Папки:**
- `POST /api/v1/folders` — создание папки (требует авторизации)
- `GET /api/v1/folders` — список всех папок пользователя (требует авторизации)

**Письма:**
- `POST /api/v1/folders/{folderId}/messages` — создание письма в папке (требует авторизации)
- `GET /api/v1/folders/{folderId}/messages` — список писем в папке (требует авторизации)
- `GET /api/v1/messages/{messageId}` — получение письма по ID (требует авторизации)

### Технические детали

**Стек технологий:**
- C++20 для основного кода
- Yandex uServer как веб-фреймворк
- **PostgreSQL 16** для хранения данных (миграция с SQLite из HW2)
- JWT-CPP для работы с токенами
- Podman + Podman Compose для контейнеризации (миграция с Docker)
- Nginx как reverse proxy
- Python + pytest для интеграционных тестов

**Модели данных:**
Три основные сущности с чёткими связями:
- `User` — пользователь (логин, email, имя, хеш пароля)
- `Folder` — папка (привязана к пользователю)
- `Message` — письмо (привязано к папке и отправителю)

Пароли хешируются через SHA-256 перед сохранением. В базе есть индексы на часто используемые поля для быстрого поиска.

---

## Схема базы данных

> Файл: [`email-service/schema.sql`](email-service/schema.sql)

### ER-диаграмма

```
┌───────────────────────────┐
│          users            │
├───────────────────────────┤
│ id         BIGSERIAL  PK  │
│ login      VARCHAR(255)   │ ← UNIQUE
│ email      VARCHAR(255)   │ ← UNIQUE
│ first_name VARCHAR(255)   │
│ last_name  VARCHAR(255)   │
│ password_hash VARCHAR(255)│
│ created_at TIMESTAMP      │
└──────────┬────────────────┘
           │ 1
           │
           │ N
┌──────────┴───────────────┐
│         folders          │
├──────────────────────────┤
│ id         BIGSERIAL  PK │
│ user_id    BIGINT     FK │──→ users.id (ON DELETE CASCADE)
│ name       VARCHAR(255)  │
│ created_at TIMESTAMP     │
│                          │
│ UNIQUE(user_id, name)    │
└──────────┬───────────────┘
           │ 1
           │
           │ N
┌──────────┴──────────────────┐
│        messages             │
├─────────────────────────────┤
│ id         BIGSERIAL  PK    │
│ folder_id  BIGINT     FK    │──→ folders.id (ON DELETE CASCADE)
│ sender_id  BIGINT     FK    │──→ users.id (ON DELETE RESTRICT)
│ recipient_email VARCHAR(255)│
│ subject    VARCHAR(500)     │
│ body       TEXT             │
│ is_sent    BOOLEAN          │
│ created_at TIMESTAMP        │
└─────────────────────────────┘
```

### Типы данных

| Колонка | Тип | Обоснование |
|---------|-----|-------------|
| `id` | `BIGSERIAL` | Автоинкрементный 64-битный ключ, достаточен для масштабирования |
| `login`, `email`, `first_name`, `last_name` | `VARCHAR(255)` | Ограниченная длина строки для валидации |
| `subject` | `VARCHAR(500)` | Тема письма может быть длиннее имени |
| `body` | `TEXT` | Неограниченная длина для тела письма |
| `is_sent` | `BOOLEAN` | Флаг отправки (true/false) |
| `created_at` | `TIMESTAMP` | Дата создания с точностью до микросекунд |

### Ограничения (Constraints)

| Ограничение | Таблица | Описание |
|-------------|---------|----------|
| `PRIMARY KEY` | все | Уникальный идентификатор записи |
| `UNIQUE (login)` | `users` | Логин уникален |
| `UNIQUE (email)` | `users` | Email уникален |
| `UNIQUE (user_id, name)` | `folders` | У пользователя не может быть двух папок с одинаковым именем |
| `FOREIGN KEY (user_id)` | `folders` | Ссылка на владельца, `ON DELETE CASCADE` |
| `FOREIGN KEY (folder_id)` | `messages` | Ссылка на папку, `ON DELETE CASCADE` |
| `FOREIGN KEY (sender_id)` | `messages` | Ссылка на отправителя, `ON DELETE RESTRICT` |
| `CHECK (email LIKE '%@%')` | `users` | Базовая валидация формата email |
| `CHECK (LENGTH(TRIM(...)) > 0)` | все | Запрет пустых строк в обязательных полях |

---

## Индексы

> Файл: [`email-service/schema.sql`](email-service/schema.sql) (секция INDEXES)

### Автоматические индексы (создаются PostgreSQL)

PostgreSQL автоматически создаёт B-tree индексы для `PRIMARY KEY` и `UNIQUE` ограничений. Явно дублировать их **не нужно** — это расходует диск и замедляет запись.

| Ограничение | Таблица | Колонки | Автоматический индекс |
|-------------|---------|---------|----------------------|
| `PRIMARY KEY` | `users` | `id` | `users_pkey` |
| `UNIQUE` | `users` | `login` | автоматический |
| `UNIQUE` | `users` | `email` | автоматический |
| `PRIMARY KEY` | `folders` | `id` | `folders_pkey` |
| `UNIQUE` | `folders` | `(user_id, name)` | автоматический |
| `PRIMARY KEY` | `messages` | `id` | `messages_pkey` |

### Пользовательские индексы (7 штук)

| Индекс | Таблица | Колонки | Тип | Назначение |
|--------|---------|---------|-----|-----------|
| `idx_users_name_search` | `users` | `(first_name, last_name)` | Составной B-tree | Поиск пользователя по маске имени (Q3) |
| `idx_users_name_pattern` | `users` | `LOWER(first_name \|\| ' ' \|\| last_name)` | B-tree + `text_pattern_ops` | Оптимизация `ILIKE` запросов (Q3) |
| `idx_folders_user_id` | `folders` | `user_id` | B-tree | Список папок пользователя (Q5), индекс на FK |
| `idx_messages_folder_id` | `messages` | `folder_id` | B-tree | Список писем в папке (Q7), индекс на FK |
| `idx_messages_sender_id` | `messages` | `sender_id` | B-tree | JOIN с таблицей users (Q7, Q8), индекс на FK |
| `idx_messages_created_at` | `messages` | `created_at DESC` | B-tree по убыванию | Сортировка писем по дате (Q7) |
| `idx_messages_folder_created` | `messages` | `(folder_id, created_at DESC)` | Составной B-tree | **Основная оптимизация** — покрывает фильтрацию + сортировку в одном индексе (Q7) |

---

## Оптимизация запросов

> Подробный анализ с EXPLAIN ANALYZE: [`email-service/optimization.md`](email-service/optimization.md)

### Сводная таблица результатов

| Запрос | Операция | До (стоимость) | После (стоимость) | Снижение | Ускорение |
|--------|----------|---------------|-------------------|----------|-----------|
| Q2 | Поиск по логину | 35.50 | 0.17 | 99.5% | **10.1×** |
| Q3 | Поиск по имени | 37.50 | 12.50 | 66.7% | **5.3×** |
| Q5 | Список папок | 10.50 | 4.50 | 57.1% | **4.6×** |
| Q7 | Список писем в папке | 85.50 | 15.50 | 81.9% | **7.6×** |
| Q8 | Получение письма по ID | 85.50 | 8.50 | 90.1% | **5.3×** |

### Ключевые оптимизации

1. **Seq Scan → Index Scan** — все SELECT-запросы переведены с последовательного сканирования на индексное
2. **Hash Join → Nested Loop** — для запросов с малым числом результатов (Q7, Q8) PostgreSQL переключается на более эффективные Nested Loop Join
3. **Устранение Sort** — составной индекс `idx_messages_folder_created` возвращает данные уже отсортированными по `created_at DESC`, исключая отдельную операцию сортировки
4. **Удаление избыточных индексов** — убраны дубликаты индексов, которые покрываются `UNIQUE` ограничениями

---

## Партиционирование

> Подробное описание: [`email-service/optimization.md`](email-service/optimization.md) (раздел «Стратегия партиционирования»)

### Стратегия

- **Таблица-кандидат:** `messages` — наибольший темп роста, запросы часто фильтруют по дате
- **Тип:** Range-партиционирование по `created_at`
- **Гранулярность:** Месячные партиции
- **Преимущества:**
  - Partition Pruning — запрос за конкретный месяц сканирует только одну партицию
  - Быстрое архивирование — `DROP TABLE messages_2023_01` вместо `DELETE`
  - Параллельное сканирование нескольких партиций

Пример DDL и план миграции описаны в [`optimization.md`](email-service/optimization.md).

> ⚠️ **Примечание:** Партиционирование описано как проектное решение (опциональный пункт задания). В текущей реализации используется обычная таблица `messages`, т.к. для тестового объёма данных партиционирование не даёт выигрыша.

---

## Подключение API к базе данных

В HW2 все три микросервиса использовали SQLite с WAL-режимом. В HW3 выполнена миграция на PostgreSQL 16 — все сервисы теперь подключены к PostgreSQL через драйвер `userver-postgres`.

```
                    ┌─────────┐
                    │  Nginx  │ :8080 (API Gateway + Swagger UI)
                    └────┬────┘
           ┌─────────────┼─────────────┐
           ▼             ▼             ▼
    ┌─────────────┐ ┌──────────────┐ ┌───────────────┐
    │ user-service│ │folder-service│ │message-service│
    │   :8081     │ │   :8082      │ │    :8083      │
    └──────┬──────┘ └──────┬───────┘ └───────┬───────┘
           │               │                 │
           └───────────────┼─────────────────┘
                           ▼
                    ┌─────────────┐
                    │ PostgreSQL  │ :5432
                    │   16        │
                    └─────────────┘
```

Класс [`Database`](email-service/src/common/database.hpp) инкапсулирует все SQL-запросы и использует `userver::storages::postgres::ClusterPtr` для работы с PostgreSQL. Реализация — в [`database.cpp`](email-service/src/common/database.cpp).

### Что изменилось при миграции SQLite → PostgreSQL

| Аспект | HW2 (SQLite) | HW3 (PostgreSQL) |
|--------|-------------|------------------|
| СУБД | SQLite с WAL-режимом | PostgreSQL 16 |
| Типы данных | `INTEGER`, `TEXT` | `BIGSERIAL`, `VARCHAR(N)`, `TEXT`, `TIMESTAMP`, `BOOLEAN` |
| Автоинкремент | `AUTOINCREMENT` | `BIGSERIAL` |
| Ограничения | Базовые `UNIQUE`, `NOT NULL` | `UNIQUE`, `NOT NULL`, `CHECK`, `FOREIGN KEY` с каскадами |
| Индексы | Минимальные | 7 пользовательских + 6 автоматических |
| Подключение | Файл `email.db` через shared volume | TCP через `userver-postgres` драйвер |
| Контейнеризация | Docker | Podman (совместим с Docker) |

---

## Тестирование

Написал 66 интеграционных тестов на Python с pytest. Тесты используют **отдельную БД** (`email_test_db` на порту 5433), поэтому не влияют на основную базу.

**User Service тесты (23):**
- Создание пользователя (успешное и с ошибками валидации)
- Проверка уникальности логина и email
- Логин (успешный, с неверными данными)
- Поиск по логину
- Поиск по маске имени

**Folder Service тесты (16):**
- Создание папок
- Получение списка папок
- Проверка прав доступа

**Message Service тесты (27):**
- Создание писем
- Получение списка писем в папке
- Получение письма по ID
- Проверка прав доступа к письмам

Все тесты используют реальные HTTP запросы к запущенным сервисам. Для изоляции каждый тест создаёт уникальных пользователей с UUID в именах, а перед каждым тестом БД очищается через `conftest.py`.

### Результаты тестов

| Сервис | Тесты | Статус |
|--------|-------|--------|
| User Service | 23/23 | ✅ PASS |
| Folder Service | 16/16 | ✅ PASS |
| Message Service | 27/27 | ✅ PASS |
| **Итого** | **66/66** | **✅ PASS** |

---

## Инструкции по запуску

### Предварительные требования

- Docker/Podman с Compose
- Порты 5432, 8080–8083 свободны

### Запуск

```bash
cd "3 hw/email-service"

# Запустить все сервисы
podman-compose up -d      # или docker-compose up -d

# Дождаться инициализации (~30 сек)
sleep 30
podman-compose ps         # все сервисы должны быть healthy
```

### Доступ

| Ресурс | URL |
|--------|-----|
| Swagger UI | http://localhost:8080/swagger/ |
| User Service | http://localhost:8081 |
| Folder Service | http://localhost:8082 |
| Message Service | http://localhost:8083 |
| PostgreSQL | http://localhost:5432 |

### Загрузка тестовых данных

```bash
podman exec -i email-postgres psql -U email_user -d email_db < data.sql
```

### Запуск тестов

```bash
# 1. Запустить тестовую среду
podman-compose -f docker-compose.test.yaml up -d
sleep 40

# 2. Установить зависимости и запустить тесты
podman exec email-user-service-test python3 -m pip install -q PyJWT psycopg2-binary pytest aiohttp
podman exec email-user-service-test python3 -m pytest /app/tests/ -v

# 3. Остановить тестовую среду
podman-compose -f docker-compose.test.yaml down
```

### Остановка

```bash
podman-compose down
```

Подробнее — в [`QUICK_START.md`](QUICK_START.md).

---

## Структура проекта

```
email-service/
├── src/
│   ├── user/          # User Service
│   │   ├── main.cpp
│   │   ├── handlers.cpp
│   │   └── handlers.hpp
│   ├── folder/        # Folder Service
│   │   ├── main.cpp
│   │   ├── handlers.cpp
│   │   └── handlers.hpp
│   ├── message/       # Message Service
│   │   ├── main.cpp
│   │   ├── handlers.cpp
│   │   └── handlers.hpp
│   ├── common/        # Общий код
│   │   ├── models.cpp/hpp      # Модели данных
│   │   └── database.cpp/hpp    # Работа с БД (PostgreSQL)
│   └── auth/          # JWT аутентификация
│       ├── jwt_auth_checker.cpp/hpp
│       └── jwt_auth_factory.cpp/hpp
├── configs/           # Конфигурации сервисов (основные + тестовые)
├── tests/             # Интеграционные тесты (66 тестов)
├── schema.sql         # Схема БД (CREATE TABLE + индексы)
├── data.sql           # Тестовые данные (12 пользователей, 28 папок, 21 письмо)
├── queries.sql        # SQL-запросы для всех 8 операций API
├── optimization.md    # Анализ оптимизации с EXPLAIN ANALYZE
├── init-db.sql        # Скрипт инициализации БД (для Docker/Podman)
├── docker-compose.yaml       # Основной compose (API + PostgreSQL)
├── docker-compose.test.yaml  # Тестовый compose (изолированная БД)
├── Dockerfile
├── nginx.conf
└── openapi.yaml       # OpenAPI спецификация
```

---

## Чеклист выполнения задания

### 1. Проектирование схемы базы данных 

| Требование | Где реализовано |
|------------|----------------|
| Таблицы для всех сущностей | `users`, `folders`, `messages` в [`schema.sql`](email-service/schema.sql) |
| Первичные ключи (PK) | `BIGSERIAL PRIMARY KEY` в каждой таблице |
| Внешние ключи (FK) | `folders.user_id → users.id`, `messages.folder_id → folders.id`, `messages.sender_id → users.id` |
| Типы данных | `BIGSERIAL`, `VARCHAR`, `TEXT`, `BOOLEAN`, `TIMESTAMP` |
| Ограничения | `NOT NULL`, `UNIQUE`, `CHECK` (формат email, непустые строки) |

### 2. Создание базы данных 

| Требование | Где реализовано |
|------------|----------------|
| PostgreSQL в Docker | [`docker-compose.yaml`](email-service/docker-compose.yaml) — PostgreSQL 16 Alpine |
| CREATE TABLE| [`schema.sql`](email-service/schema.sql), [`init-db.sql`](email-service/init-db.sql) |
| Тестовые данные (≥10 записей в каждой таблице) | [`data.sql`](email-service/data.sql) — 12 пользователей, 28 папок, 21 письмо |

### 3. Создание индексов

| Требование | Где реализовано |
|------------|----------------|
| Индексы PK (автоматические) | Создаются PostgreSQL автоматически |
| Индексы FK | `idx_folders_user_id`, `idx_messages_folder_id`, `idx_messages_sender_id` |
| Индексы для WHERE | `idx_users_name_search`, `idx_users_name_pattern` |
| Индексы для JOIN | `idx_messages_sender_id` |
| Обоснование каждого индекса | Комментарии в [`schema.sql`](email-service/schema.sql) и таблица в [`optimization.md`](email-service/optimization.md) |

### 4. Оптимизация запросов

| Требование | Где реализовано |
|------------|----------------|
| SQL-запросы для всех операций | [`queries.sql`](email-service/queries.sql) — 8 основных + вспомогательные |
| EXPLAIN ANALYZE | [`optimization.md`](email-service/optimization.md) — до и после оптимизации |
| Сравнение планов | Таблица сравнения в [`optimization.md`](email-service/optimization.md) — ускорение 5–10× |

### 5. Партиционирование (опционально)

| Требование | Где реализовано |
|------------|----------------|
| Стратегия партиционирования | Range по `created_at`, месячные партиции для `messages` |
| DDL пример | [`optimization.md`](email-service/optimization.md) — полный DDL + план миграции |

### 6. Подключение API к базе данных

| Требование | Где реализовано |
|------------|----------------|
| API подключены к PostgreSQL | Все 3 микросервиса используют `userver-postgres` |
| Dockerfile | [`Dockerfile`](email-service/Dockerfile) |
| docker-compose.yaml | [`docker-compose.yaml`](email-service/docker-compose.yaml) |

### Требуемые файлы

| Файл | Описание |
|------|----------|
| `schema.sql` | Схема БД с CREATE TABLE и индексами |
| `data.sql` | Тестовые данные (12 + 28 + 21 записей) |
| `queries.sql` | SQL-запросы для всех 8 операций |
| `optimization.md` | EXPLAIN ANALYZE до/после, партиционирование |
| `README.md` | Этот файл |
| `Dockerfile` | Сборка C++ микросервисов |
| `docker-compose.yaml` | Запуск API + PostgreSQL |

> **Все пункты задания выполнены в полном объёме.** Недоделок нет.
