# Быстрый старт

## Запуск проекта

```bash
cd "5 hw/email-service"
podman-compose up -d
sleep 20
podman-compose ps
```

Все сервисы должны быть в статусе `healthy`.

## Доступ к сервисам

### API Endpoints
- **Swagger UI:** http://localhost:8080/swagger/
- **User Service:** http://localhost:8081
- **Folder Service:** http://localhost:8082
- **Message Service:** http://localhost:8083
- **MongoDB:** localhost:27017

### Мониторинг
- **Grafana:** http://localhost:3000 (admin/admin)
- **Prometheus:** http://localhost:9090
- **Cache & Rate Limiting Metrics:** http://localhost:8081/cache-rate-limiting-metrics

## Загрузить тестовые данные

```bash
podman exec -i email-mongodb mongosh -u email_user -p email_pass --authenticationDatabase email_db email_db < data.js
```

Результат: 12 users, 36 folders, 47 messages.

## Тестирование кэширования и Rate Limiting

### Запуск автоматических тестов

```bash
cd "5 hw/email-service/tests"
./test_cache_rate_limiting.sh
```

Этот скрипт проверяет:
1. **Rate Limiting** - ограничение запросов (5 запросов burst для регистрации)
2. **Caching** - эффективность кэширования при повторных запросах

### Нагрузочное тестирование

```bash
cd "5 hw/email-service/tests"
./load_test.sh
```

Этот скрипт выполняет комплексное нагрузочное тестирование с использованием Apache Bench (ab):

**Тесты включают:**
1. **User Registration** - создание пользователей под нагрузкой
2. **User Login** - вход в систему под нагрузкой
3. **Find User by Login** - поиск пользователя по логину (с кэшированием)
4. **List Folders** - получение списка папок (с кэшированием)
5. **List Messages** - получение списка сообщений (с кэшированием)

**Результаты тестов:**
- Сохраняются в директорию `./load_test_results/`
- Включают метрики производительности (RPS, latency, etc.)
- Генерируют графики в формате gnuplot

**Параметры тестирования:**
- Количество запросов: 1000
- Уровень параллелизма: 10-50
- Различные сценарии нагрузки для разных endpoints

## Мониторинг в Grafana

### Доступ к дашбордам

1. Откройте Grafana: http://localhost:3000
2. Войдите с учетными данными admin/admin
3. Перейдите в раздел Dashboards
4. Выберите "Email Service - Cache & Rate Limiting Dashboard"

### Метрики дашборда

Дашборд показывает следующие метрики:

1. **Cache Hit Rate** - процент попаданий в кэш
2. **Cache Hits/Misses Rate** - скорость попаданий и промахов кэша
3. **Rate Limit Violations** - количество нарушений rate limiting
4. **Request Rate (Allowed vs Denied)** - скорость разрешенных и отклоненных запросов
5. **Cache Size** - текущий размер кэша
6. **Cache Evictions Rate** - скорость вытеснения из кэша
7. **Active Rate Limit Buckets** - количество активных bucket'ов rate limiting

### Просмотр метрик напрямую

```bash
# Получить все метрики кэша и rate limiting
curl http://localhost:8081/cache-rate-limiting-metrics
```

Пример вывода:
```
# HELP cache_hits_total Total number of cache hits
# TYPE cache_hits_total counter
cache_hits_total{cache="user"} 9

# HELP cache_misses_total Total number of cache misses
# TYPE cache_misses_total counter
cache_misses_total{cache="user"} 1

# HELP rate_limit_denied_requests_total Total number of requests denied by rate limiter
# TYPE rate_limit_denied_requests_total counter
rate_limit_denied_requests_total{endpoint="user-registration"} 2
```

## Проверка эффективности оптимизации

### Кэширование

Эффективность кэширования измеряется как:
```
Cache Hit Rate = cache_hits / (cache_hits + cache_misses)
```

Хорошее значение: > 80%
Отличное значение: > 90%

### Rate Limiting

Эффективность rate limiting измеряется как:
```
Blocked Requests = rate_limit_denied_requests_total
```

Цель: блокировать все запросы, превышающие лимит.

## Остановка контейнеров

```bash
podman-compose down
```

Для удаления volumes:
```bash
podman-compose down -v
```

## Решение проблем

### Grafana не запускается

```bash
# Проверить логи Grafana
podman logs email-grafana

# Перезапустить Grafana
podman-compose restart grafana
```

### Метрики не обновляются

```bash
# Проверить, что metrics endpoint доступен
curl http://localhost:8081/cache-rate-limiting-metrics

# Проверить логи user service
podman logs email-user-service
```

### Дашборд показывает "No Data"

1. Убедитесь, что Prometheus запущен: http://localhost:9090
2. Проверьте, что Prometheus скрапит метрики:
   - Откройте http://localhost:9090/targets
   - Убедитесь, что все targets в состоянии "UP"
3. Проверьте, что datasource в Grafana настроен правильно:
   - Settings → Data Sources → Prometheus
   - URL: http://prometheus:9090

### Rate Limiting не работает

```bash
# Проверить метрики rate limiting
curl http://localhost:8081/cache-rate-limiting-metrics | grep rate_limit

# Убедитесь, что компонент rate-limiter загружен
podman logs email-user-service | grep "RateLimiterComponent"
```

## Дополнительно

- [`TASK_INFO.md`](TASK_INFO.md) — формулировка задания
- [`README.md`](README.md) — подробная документация проекта
