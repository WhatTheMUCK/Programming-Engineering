# Быстрый старт

## Запуск проекта (для разработки)

```bash
cd "4 hw/email-service"
podman-compose up -d
sleep 20
podman-compose ps
```

Все сервисы должны быть в статусе `healthy`.

## Доступ к API

- **Swagger UI:** http://localhost:8080/swagger/
- **User Service:** http://localhost:8081
- **Folder Service:** http://localhost:8082
- **Message Service:** http://localhost:8083
- **MongoDB (основная БД):** localhost:27017

## Загрузить тестовые данные в основную БД

```bash
podman exec -i email-mongodb mongosh -u email_user -p email_pass --authenticationDatabase email_db email_db < data.js
```

Результат: 12 users, 36 folders, 47 messages.

## Запустить MongoDB запросы (CRUD операции)

```bash
podman exec -i email-mongodb mongosh -u email_user -p email_pass --authenticationDatabase email_db email_db < queries.js
```

Запросы включают:
- **CREATE**: Q1 (User), Q4 (Folder), Q6 (Message)
- **READ**: Q2 (Find by login), Q3 (Search by name), Q5 (List folders), Q7 (List messages), Q8 (Get by ID)
- **UPDATE**: updateOne, $inc, embedded document
- **DELETE**: deleteOne, deleteMany
- **AGGREGATION**: messages per folder, messages by sender
- **ADVANCED**: $in, $gt, $lt, $exists operators

## Валидация схемы

```bash
podman exec -i email-mongodb mongosh -u email_user -p email_pass --authenticationDatabase email_db email_db < validation.js
```

Тесты:
- Test 1: Valid document
- Test 2: Missing required field
- Test 3: Wrong type
- Test 4: Login too short
- Test 5: Invalid email pattern
- Test 6: Password hash too short
- Test 7: Extra field allowed

## Запустить тесты (с изолированной БД)

**ВАЖНО:** Тесты используют **отдельную БД** (`email_test_db`), поэтому они **НЕ влияют на основную БД**.

### Быстрый способ

```bash
# 1. Запустить тестовую среду
podman-compose -f docker-compose.test.yaml up -d
sleep 40

# 2. Установить зависимости и запустить тесты
podman exec email-user-service-test python3 -m pip install -q PyJWT pytest pytest-asyncio aiohttp pymongo yandex-taxi-testsuite
podman exec email-user-service-test python3 -m pytest /app/tests/ -v

# 3. Остановить
podman-compose -f docker-compose.test.yaml down
```

### Запустить отдельные тесты

```bash
# User Service тесты
podman exec email-user-service-test python3 -m pytest /app/tests/test_user_service.py -v

# Folder Service тесты
podman exec email-folder-service-test python3 -m pytest /app/tests/test_folder_service.py -v

# Message Service тесты
podman exec email-message-service-test python3 -m pytest /app/tests/test_message_service.py -v
```

## Остановка контейнеров

```bash
# Остановить основные сервисы
podman-compose down

# Остановить тестовую среду
podman-compose -f docker-compose.test.yaml down
```

## Типичный рабочий процесс

### Для разработки и ручного тестирования

```bash
# 1. Запустить основные сервисы
podman-compose up -d

# 2. Загрузить тестовые данные
podman exec -i email-mongodb mongosh email_db < data.js

# 3. Открыть Swagger UI и тестировать API
# http://localhost:8080/swagger/

# 4. Остановить
podman-compose down
```

### Для автоматического тестирования

```bash
# 1. Запустить тестовую среду
podman-compose -f docker-compose.test.yaml up -d
sleep 40

# 2. Установить зависимости и запустить тесты
podman exec email-user-service-test python3 -m pip install -q PyJWT pytest pytest-asyncio aiohttp pymongo yandex-taxi-testsuite
podman exec email-user-service-test python3 -m pytest /app/tests/ -v

# 3. Остановить
podman-compose -f docker-compose.test.yaml down
```

## Решение проблем

### Ошибка: `cannot remove container as it is running`

```bash
# Остановить контейнеры перед удалением
podman-compose stop
podman-compose down

# Или удалить pod принудительно
podman pod rm -f <pod_id>
```

### Тесты не запускаются

```bash
# Убедитесь, что тестовая среда запущена
podman ps | grep test

# Проверьте логи MongoDB
podman logs email-mongo-test

# Проверьте логи сервисов
podman logs email-user-service-test
podman logs email-folder-service-test
podman logs email-message-service-test

# Убедитесь, что прошло 40 секунд инициализации
sleep 40
```

### Ошибка `Component with name 'mongo' found in static config`

Используйте актуальные тестовые конфиги:
- [`static_config_user_test.yaml`](4%20hw_backup/email-service/configs/static_config_user_test.yaml)
- [`static_config_folder_test.yaml`](4%20hw_backup/email-service/configs/static_config_folder_test.yaml)
- [`static_config_message_test.yaml`](4%20hw_backup/email-service/configs/static_config_message_test.yaml)

В них имя компонента должно совпадать с зарегистрированным [`mongo-db`](4%20hw_backup/email-service/src/user/main.cpp:16).

## Дополнительно

- [`TASK_INFO.md`](4%20hw_backup/TASK_INFO.md) — формулировка задания
- [`schema_design.md`](4%20hw_backup/email-service/schema_design.md) — проектирование MongoDB-схемы
- [`queries.js`](4%20hw_backup/email-service/queries.js) — MongoDB-запросы
- [`validation.js`](4%20hw_backup/email-service/validation.js) — валидация схем
- [`openapi.yaml`](4%20hw_backup/email-service/openapi.yaml) — OpenAPI спецификация
