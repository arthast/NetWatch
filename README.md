# NetWatch

NetWatch - платформа мониторинга сетевых сервисов на C++ с использованием
userver. Проект разделен на четыре runtime-сервиса: внешний HTTP API живет в
`api-gateway`, targets обслуживаются в `target-service`, checks и scheduler - в
`monitor-service`, а alert lifecycle вынесен в отдельный `alert-service`.
Внутреннее взаимодействие между сервисами идет по gRPC.

## Сейчас реализовано

- `api-gateway` с внешним HTTP API, Swagger UI и OpenAPI specification.
- `target-service` с gRPC API для хранения и валидации targets.
- `target-service` владеет target-моделью и валидацией.
- `monitor-service` владеет check-моделью.
- Ручные HTTP/TCP проверки.
- История checks и статус target по последней проверке.
- Alert lifecycle: `target_down` создается при падении и закрывается при восстановлении.
- Scheduler, который периодически запускает проверки active targets.
- `monitor-service` с gRPC API для checks.
- `alert-service` с gRPC API для alerts и собственной PostgreSQL базой.
- Service-owned PostgreSQL migrations, unit-тесты и Docker Compose для demo-запуска.

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
        +--------------------+--------------------+
        |                    |                    |
        v                    v                    v
target-service        monitor-service        alert-service
  |-- targets CRUD      |-- manual checks      |-- alert lifecycle
  |-- validation        |-- scheduler          |-- active/resolved alerts
  |-- repository        |-- check history      |-- repository
                       |
                       | gRPC
                       +--------------------+
                       |                    |
                       v                    v
                 target-service        alert-service

api-gateway     ---- no direct DB access
target-service  ---- target-postgres
monitor-service ---- monitor-postgres
alert-service   ---- alert-postgres
```

Target-типы и валидация принадлежат `target-service`, check-модель принадлежит
`monitor-service`, gRPC clients - в `libs/target-client`,
`libs/monitor-client` и `libs/alert-client`, gRPC контракты - в
`proto/netwatch`. Alert domain теперь принадлежит `alert-service`, клиентский
alert DTO живет рядом с `alert-client`, а alert lifecycle принимает собственные
snapshot-сообщения из `alert_service.proto` без импорта target/check контрактов.
`target-client` теперь содержит только клиентские DTO и gRPC mapping, а PATCH
target применяется и валидируется внутри `target-service`.
`monitor-client` содержит клиентские check DTO и gRPC mapping, а check domain
остается внутри `monitor-service`.

Service-owned код теперь лежит внутри владельцев: target repository - в
`services/target-service/src`, check storage и runner - в
`services/monitor-service/src`, alert storage и lifecycle - в
`services/alert-service/src`.

PostgreSQL DDL разнесен по service-owned migrations:

- `services/target-service/postgresql/migrations` владеет таблицей `targets`.
- `services/monitor-service/postgresql/migrations` владеет таблицей
  `check_results`.
- `services/alert-service/postgresql/migrations` владеет таблицей `alerts`.

В Docker Compose это уже отдельные runtime databases: `target-service`
подключается к `target-postgres/target_service_db`, а `monitor-service` - к
`monitor-postgres/monitor_service_db`, `alert-service` - к
`alert-postgres/alert_service_db`. Между этими базами нет FK и общих таблиц.

В Docker Compose `api-gateway` вызывает `target-service`, `monitor-service` и
`alert-service` по gRPC. `monitor-service` ходит в `target-service`, чтобы
получать targets для ручных проверок и scheduler, и в `alert-service`, чтобы
передавать результат проверки в alert lifecycle.

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

Dev Compose собирает сервисные бинарники внутри контейнеров при старте. Это
удобно для локальной разработки:

```bash
docker compose up --build
```

Сервисы будут доступны так:

- `api-gateway`: `http://localhost:8081` - внешний HTTP API, Swagger, checks, alerts.
- `target-service`: `http://localhost:8082/ping` - healthcheck; бизнес API у него gRPC на внутреннем `target-service:8090`.
- `monitor-service`: внутренний сервис checks/scheduler; HTTP наружу не публикуется, gRPC внутри Compose на `monitor-service:8091`.
- `alert-service`: внутренний сервис alert lifecycle; HTTP наружу не публикуется, gRPC внутри Compose на `alert-service:8092`.
- `target-postgres`: `localhost:15432`, база `target_service_db`.
- `monitor-postgres`: `localhost:15434`, база `monitor_service_db`.
- `alert-postgres`: `localhost:15435`, база `alert_service_db`.

Полезные страницы:

- `http://localhost:8081/docs` - Swagger UI.
- `http://localhost:8081/openapi.json` - OpenAPI JSON.

Если нужно полностью пересоздать demo-базу:

```bash
docker compose down -v
docker compose up --build
```

Команда `down -v` удалит локальные demo-данные PostgreSQL.

Production-like Compose использует уже собранные Docker images:

```bash
docker compose -f docker-compose.images.yml build
docker compose -f docker-compose.images.yml up
```

По умолчанию локальные images называются:

- `netwatch/api-gateway:local`
- `netwatch/target-service:local`
- `netwatch/monitor-service:local`
- `netwatch/alert-service:local`

Для запуска опубликованных images можно переопределить переменные:

```bash
NETWATCH_API_GATEWAY_IMAGE=ghcr.io/<owner>/<repo>/api-gateway:<tag> \
NETWATCH_TARGET_SERVICE_IMAGE=ghcr.io/<owner>/<repo>/target-service:<tag> \
NETWATCH_MONITOR_SERVICE_IMAGE=ghcr.io/<owner>/<repo>/monitor-service:<tag> \
NETWATCH_ALERT_SERVICE_IMAGE=ghcr.io/<owner>/<repo>/alert-service:<tag> \
docker compose -f docker-compose.images.yml up
```

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

Сейчас автоматизированные тесты разделены по ownership: target validation живет в
`services/target-service` как `netwatch_target_service_unittest`, monitor acceptance
сценарии остаются в `services/monitor-service/tests`, а внешний HTTP/API gateway
контракт проверяется integration flow. Flow поднимает Docker Compose и проверяет контур
`api-gateway -> gRPC -> target-service/monitor-service/alert-service -> PostgreSQL`.

В контейнере devcontainer или userver-окружении:

```bash
cd services/monitor-service
make test-debug
```

Через Docker Compose:

```bash
docker compose run --rm --no-deps --user 1000:1000 \
  --workdir /workspace \
  monitor-service \
  bash -lc 'cmake -S . -B build-compose-test-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DUSERVER_FEATURE_GRPC=ON -DUSERVER_FEATURE_POSTGRESQL=ON -DUSERVER_SANITIZE="addr;ub" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build-compose-test-debug -j 1 --target netwatch_target_service_unittest monitor_service && cd build-compose-test-debug && ctest --output-on-failure -R netwatch_target_service_unittest'
```

Собрать все сервисы из корня репозитория:

```bash
docker compose run --rm --no-deps --workdir /workspace monitor-service \
  bash -lc 'cmake -S . -B build-compose-root-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DUSERVER_FEATURE_GRPC=ON -DUSERVER_FEATURE_POSTGRESQL=ON -DUSERVER_SANITIZE="addr;ub" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build-compose-root-debug -j 1 --target api_gateway monitor_service alert_service target_service'
```

Запустить внешний integration flow через gateway:

```bash
./tests/integration/run_api_gateway_flow.py
```

Быстрая локальная проверка во время разделения сервисов:

```bash
./scripts/quick_check.sh build
```

Для повторной проверки HTTP/gRPC flow без пересоздания Compose на каждый прогон:

```bash
./scripts/quick_check.sh flow-keep
./scripts/quick_check.sh flow-skip
./scripts/quick_check.sh down
```

Запустить тот же flow против production-like images:

```bash
docker compose -f docker-compose.images.yml build
./tests/integration/run_api_gateway_flow.py \
  --compose-file docker-compose.images.yml \
  --no-build
```

## CI/CD

GitHub Actions workflow находится в `.github/workflows/ci.yml`.

На pull request и push он выполняет:

- root-сборку `api_gateway`, `target_service`, `monitor_service`, `alert_service`;
- `netwatch_target_service_unittest` через отдельный target-service job;
- отдельные service build jobs для `api-gateway`, `target-service`,
  `monitor-service`, `alert-service`;
- внешний integration flow через dev-compose;
- сборку production-like Docker images;
- integration flow через `docker-compose.images.yml`.

На push в `main` или `dev/**` workflow дополнительно публикует images в GHCR:

- `ghcr.io/<owner>/<repo>/api-gateway:<sha|branch>`
- `ghcr.io/<owner>/<repo>/target-service:<sha|branch>`
- `ghcr.io/<owner>/<repo>/monitor-service:<sha|branch>`
- `ghcr.io/<owner>/<repo>/alert-service:<sha|branch>`
