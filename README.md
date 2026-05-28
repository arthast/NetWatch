# NetWatch

NetWatch - платформа мониторинга сетевых сервисов на C++ с использованием
userver. Проект уже разделен на первые микросервисы: внешний HTTP API живет в
`api-gateway`, targets обслуживаются отдельным `target-service`, а checks,
alerts и scheduler находятся в `monitor-service`. Внутреннее взаимодействие
между сервисами идет по gRPC.

## Сейчас реализовано

- `api-gateway` с внешним HTTP API, Swagger UI и OpenAPI specification.
- `target-service` с gRPC API для хранения и валидации targets.
- Общая библиотека `target-core` для target-модели, валидации и PostgreSQL repository.
- Общая библиотека `monitor-core` для check/alert доменных моделей.
- Ручные HTTP/TCP проверки.
- История checks и статус target по последней проверке.
- Alert lifecycle: `target_down` создается при падении и закрывается при восстановлении.
- Scheduler, который периодически запускает проверки active targets.
- `monitor-service` с gRPC API для checks и alerts.
- PostgreSQL schema, unit-тесты и Docker Compose для demo-запуска.

## Архитектура текущей версии

```text
Client / Swagger / curl
        |
        | HTTP
        v
api-gateway
  |-- HTTP JSON API
  |-- Swagger UI and OpenAPI spec
  |-- common: shared HTTP/JSON/path helpers
        | gRPC
        +--------------------+
        |                    |
        v                    v
target-service        monitor-service
  |-- targets CRUD      |-- manual checks
  |-- validation        |-- scheduler
  |-- repository        |-- check history/status
                       |-- alert lifecycle
                       |
                       | gRPC
                       v
                 target-service

api-gateway     ---- no direct DB access
monitor-service ---- PostgreSQL
target-service  ---- PostgreSQL
```

Общие target-типы лежат в `libs/target-core`, общие check/alert модели - в
`libs/monitor-core`, gRPC контракты - в `proto/netwatch`.

В Docker Compose `api-gateway` вызывает `target-service` и `monitor-service`
по gRPC. `monitor-service` тоже ходит в `target-service` по gRPC, чтобы получать
targets для ручных проверок и scheduler.

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

Сервисы будут доступны так:

- `api-gateway`: `http://localhost:8081` - внешний HTTP API, Swagger, checks, alerts.
- `target-service`: `http://localhost:8082/ping` - healthcheck; бизнес API у него gRPC на внутреннем `target-service:8090`.
- `monitor-service`: внутренний сервис checks/alerts; HTTP наружу не публикуется, gRPC внутри Compose на `monitor-service:8091`.
- PostgreSQL: `localhost:15432`.

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

Создать HTTP target, который проверяет `monitor-service` изнутри контейнера:

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

Сейчас после выделения `api-gateway` автоматизированно запускается unit-тест
валидации targets. Старые HTTP integration tests нужно перенести на новый
gateway-контур отдельным шагом.

В контейнере devcontainer или userver-окружении:

```bash
cd services/monitor-service
make test-debug
```

Через Docker Compose:

```bash
docker compose run --rm --no-deps --user 1000:1000 \
  --workdir /workspace/services/monitor-service \
  monitor-service \
  bash -lc 'cmake -S . -B build-compose-test-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DUSERVER_FEATURE_GRPC=ON -DUSERVER_FEATURE_POSTGRESQL=ON -DUSERVER_SANITIZE="addr;ub" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build-compose-test-debug -j 4 --target monitor_service_unittest monitor_service && cd build-compose-test-debug && ctest --output-on-failure'
```

Собрать все сервисы из корня репозитория:

```bash
docker compose run --rm --no-deps --workdir /workspace monitor-service \
  bash -lc 'cmake -S . -B build-compose-root-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DUSERVER_FEATURE_GRPC=ON -DUSERVER_FEATURE_POSTGRESQL=ON -DUSERVER_SANITIZE="addr;ub" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build-compose-root-debug -j 1 --target api_gateway monitor_service target_service'
```
