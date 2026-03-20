# Быстрый старт

## Запуск проекта

```bash
cd "email-service"
docker-compose up --build -d
```

Подождите 15-20 секунд пока сервисы запустятся.

## Проверка статуса

```bash
docker-compose ps
```

Все сервисы должны быть в статусе `healthy`.

## Тестирование

Запуск всех тестов:

```bash
docker exec email-user-service python3 -m pytest /app/tests/ -v
```

Или по отдельности:

```bash
docker exec email-user-service python3 -m pytest /app/tests/test_user_service.py -v
docker exec email-folder-service python3 -m pytest /app/tests/test_folder_service.py -v
docker exec email-message-service python3 -m pytest /app/tests/test_message_service.py -v
```

## Swagger UI

Откройте в браузере: http://localhost:8080/swagger/

## Остановка

```bash
docker-compose down
```

## Архитектура

- **User Service** (8081) — пользователи и аутентификация
- **Folder Service** (8082) — почтовые папки
- **Message Service** (8083) — письма
- **Nginx** (8080) — API Gateway

Все запросы идут через Nginx на порт 8080.
