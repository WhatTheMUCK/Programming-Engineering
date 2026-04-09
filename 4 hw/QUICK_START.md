# Быстрый старт

## Запуск проекта (для разработки)

```bash
cd "3 hw/email-service"
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
- **PostgreSQL (основная БД):** localhost:5432

## Загрузить тестовые данные в основную БД

```bash
podman exec -i email-postgres psql -U email_user -d email_db < data.sql
```

## Запустить тесты (с изолированной БД)

**ВАЖНО:** Тесты используют **отдельную БД** (`email_test_db`), поэтому они **НЕ влияют на основную БД**.

### Быстрый способ

```bash
# 1. Запустить тестовую среду
podman-compose -f docker-compose.test.yaml up -d
sleep 40

# 2. Запустить тесты
podman exec email-user-service-test python3 -m pip install -q PyJWT psycopg2-binary pytest aiohttp
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
podman exec -i email-postgres psql -U email_user -d email_db < data.sql

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
podman exec email-user-service-test python3 -m pip install -q PyJWT psycopg2-binary pytest aiohttp
podman exec email-user-service-test python3 -m pytest /app/tests/ -v

# 3. Остановить
podman-compose -f docker-compose.test.yaml down
```

## Решение проблем

### Ошибка: "cannot remove container as it is running"

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

# Проверьте логи БД
podman logs email-postgres-test

# Убедитесь, что прошло 40 секунд инициализации
sleep 40
```

## Дополнительно

- **README.md** — полное описание проекта
- **TESTING_GUIDE.md** — подробное руководство по тестированию
- **optimization.md** — анализ оптимизации запросов
- **openapi.yaml** — OpenAPI спецификация
