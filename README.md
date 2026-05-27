# NetWatch

NetWatch - платформа мониторинга сетевых сервисов на C++ с использованием
userver. Сейчас проект развивается как modular monolith: один сервис
`monitor-service`, внутри которого код разделен на модули `targets`, `checks`,
`alerts`, `web` и `common`.

## Сейчас реализовано

- CRUD для HTTP/TCP targets.
- Ручные HTTP/TCP проверки.
- История checks и статус target по последней проверке.
- Alert lifecycle: `target_down` создается при падении и закрывается при восстановлении.
- Scheduler, который периодически запускает проверки active targets.
- Swagger UI и OpenAPI specification.
- PostgreSQL schema, testsuite-тесты и Docker Compose для demo-запуска.

## Архитектура текущей версии

```text
Client / Swagger / curl
        |
        | HTTP
        v
monitor-service
  |-- targets: CRUD monitoring targets
  |-- checks: HTTP/TCP checks, history, current status
  |-- alerts: target_down lifecycle
  |-- web: Swagger UI and OpenAPI spec
  |-- common: shared HTTP/JSON/path helpers
        |
        v
PostgreSQL
```

## Основные endpoint'ы

- `GET /ping`
- `GET /docs`
- `GET /openapi.json`
- `POST /api/v1/targets`
- `GET /api/v1/targets`
- `GET /api/v1/targets/{id}`
- `PATCH /api/v1/targets/{id}`
- `DELETE /api/v1/targets/{id}`
- `POST /api/v1/targets/{id}/check`
- `GET /api/v1/targets/{id}/checks`
- `GET /api/v1/targets/{id}/status`
- `GET /api/v1/alerts`
- `GET /api/v1/alerts/active`

## Demo через Docker Compose

```bash
docker compose up --build
```

Сервис будет доступен на `http://localhost:8081`, PostgreSQL - на `localhost:15432`.

Полезные страницы:

- `http://localhost:8081/docs` - Swagger UI.
- `http://localhost:8081/openapi.json` - OpenAPI JSON.

Если нужно полностью пересоздать demo-базу:

```bash
docker compose down -v
docker compose up --build
```

Команда `down -v` удалит локальные demo-данные PostgreSQL.

## Demo сценарий через curl

```bash
BASE=http://localhost:8081
```

Проверить, что сервис жив:

```bash
curl -i "$BASE/ping"
```

Создать HTTP target, который проверяет сам `monitor-service` изнутри контейнера:

```bash
HTTP_ID=$(
  curl -s -X POST "$BASE/api/v1/targets" \
    -H 'Content-Type: application/json' \
    -d '{
      "name": "NetWatch self ping",
      "type": "http",
      "url": "http://localhost:8080/ping",
      "method": "GET",
      "expected_status_code": 200,
      "interval_seconds": 30,
      "timeout_ms": 1000
    }' | jq -r '.id'
)
```

Запустить ручную проверку и посмотреть историю:

```bash
curl -s -X POST "$BASE/api/v1/targets/$HTTP_ID/check" | jq
curl -s "$BASE/api/v1/targets/$HTTP_ID/status" | jq
curl -s "$BASE/api/v1/targets/$HTTP_ID/checks" | jq
```

Создать TCP target на закрытый порт, чтобы получить `down` и active alert:

```bash
TCP_ID=$(
  curl -s -X POST "$BASE/api/v1/targets" \
    -H 'Content-Type: application/json' \
    -d '{
      "name": "Broken TCP target",
      "type": "tcp",
      "host": "localhost",
      "port": 1,
      "interval_seconds": 30,
      "timeout_ms": 500
    }' | jq -r '.id'
)

curl -s -X POST "$BASE/api/v1/targets/$TCP_ID/check" | jq
curl -s "$BASE/api/v1/alerts/active" | jq
```

Починить TCP target, направив его на открытый порт сервиса, и закрыть alert:

```bash
curl -s -X PATCH "$BASE/api/v1/targets/$TCP_ID" \
  -H 'Content-Type: application/json' \
  -d '{"port": 8080}' | jq

curl -s -X POST "$BASE/api/v1/targets/$TCP_ID/check" | jq
curl -s "$BASE/api/v1/alerts/active" | jq
curl -s "$BASE/api/v1/alerts" | jq
```

## Тесты

В контейнере devcontainer или userver-окружении:

```bash
cd services/monitor-service
make test-debug
```

Через Docker Compose:

```bash
docker compose run --rm --no-deps monitor-service \
  bash -lc 'cmake -S . -B build-compose-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DUSERVER_FEATURE_POSTGRESQL=ON -DUSERVER_SANITIZE="addr;ub" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build-compose-debug -j 4 && chmod -R a+rwX build-compose-debug .ccache'

docker compose run --rm --no-deps --user 1000:1000 monitor-service \
  bash -lc 'cd build-compose-debug && USERVER_ENABLE_STACK_USAGE_MONITOR=0 ctest -V'
```
