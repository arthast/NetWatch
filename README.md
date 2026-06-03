# NetWatch

NetWatch - платформа для мониторинга HTTP/TCP сервисов, написанная на
C++ и userver. Проект построен как микросервисная система: внешний HTTP API
отделен от доменных сервисов, каждый сервис владеет своей базой данных,
а внутреннее взаимодействие идет по gRPC.

Публичный стенд:

- Swagger UI: `http://158.160.233.79/docs`
- OpenAPI JSON: `http://158.160.233.79/openapi.json`

## Стек

- C++ / userver
- gRPC / Protocol Buffers
- Kafka
- PostgreSQL
- Docker / Docker Compose
- CMake / Ninja
- Swagger / OpenAPI
- GitHub Actions

## Что умеет NetWatch

- хранить HTTP и TCP targets
- запускать ручные проверки target через API
- выполнять периодические проверки active targets через scheduler
- сохранять историю check results
- возвращать текущий статус target
- создавать active alert при падении target
- закрывать alert при восстановлении target
- отдавать Swagger UI и OpenAPI contract
- собираться и проверяться через CI/CD

Gateway нормализует ошибки внутренних gRPC-сервисов в HTTP 502/504, а
`monitor-service` сохраняет результат проверки даже при временной недоступности
`alert-service`. Scheduler использует PostgreSQL lease на target, поэтому
несколько реплик `monitor-service` не запускают одну и ту же scheduled-проверку
одновременно. Основной мониторинг деградирует мягко и не падает целиком из-за
вторичной подсистемы.

## Архитектура

```mermaid
flowchart LR
    client["Client / Swagger / curl"] -->|HTTP JSON| gateway["api-gateway"]

    gateway -->|gRPC| target["target-service"]
    gateway -->|gRPC| monitor["monitor-service"]
    gateway -->|gRPC| alert["alert-service"]

    monitor -->|gRPC: read targets| target
    monitor -->|gRPC: check snapshots| alert
    alert -->|Kafka: alert.opened / alert.resolved| kafka["Kafka<br/>netwatch.alert.events.v1"]
    kafka -->|consume alert events| notification["notification-service"]
    notification -->|SMTP / email API| email["Email provider"]

    target --> target_db[("target-postgres<br/>targets")]
    monitor --> monitor_db[("monitor-postgres<br/>check_results")]
    alert --> alert_db[("alert-postgres<br/>alerts<br/>alert_outbox_events")]
```

Снаружи открыт только `api-gateway`. Остальные сервисы считаются внутренними и
доступны друг другу по gRPC внутри runtime-сети. События alert lifecycle
доставляются асинхронно: `alert-service` пишет их в outbox и публикует в Kafka,
а `notification-service` будет читать topic и отправлять уведомления.

## Сервисы

| Service | Ответственность | Storage |
| --- | --- | --- |
| `api-gateway` | HTTP API, Swagger UI, OpenAPI, JSON/gRPC mapping | нет |
| `target-service` | target domain, CRUD, PATCH, validation | `target_service_db` |
| `monitor-service` | check runner, scheduler, check history, target status | `monitor_service_db` |
| `alert-service` | alert lifecycle, active/resolved alerts, Kafka outbox events | `alert_service_db` |
| `notification-service` | Kafka consumer для alert events, email notification delivery | будет определено при реализации |

### `api-gateway`

Единая публичная точка входа. Gateway принимает HTTP/JSON, вызывает внутренние
gRPC-сервисы и возвращает клиенту нормальные HTTP-ответы. Он не подключается к
PostgreSQL и не содержит доменных правил.

### `target-service`

Владелец target domain. Сервис хранит targets, валидирует HTTP/TCP параметры и
сам применяет PATCH к текущему состоянию target.

### `monitor-service`

Владелец check domain. Сервис получает targets через `target-service`, выполняет
HTTP/TCP проверки, сохраняет историю и возвращает последний статус target.
Scheduler периодически обходит active targets и запускает проверки без участия
внешнего клиента.

### `alert-service`

Владелец alert lifecycle. Сервис получает snapshot target и previous/current
check results, открывает `target_down` alert при падении и закрывает его после
восстановления. После изменения alert lifecycle сервис записывает событие в
`alert_outbox_events` и публикует его в Kafka topic `netwatch.alert.events.v1`.

### `notification-service`

Сервис уведомлений. Он будет читать события `alert.opened` и `alert.resolved`
из Kafka, применять настройки получателей/каналов и отправлять email через
SMTP или внешний email API. Этот сервис не должен вызываться синхронно из
`monitor-service` или `alert-service`: доставка уведомлений живет отдельно от
основного мониторинга.

## Границы владения в коде

```text
services/
  api-gateway/       public HTTP API, Swagger, OpenAPI, JSON mapping
  target-service/    target domain, validation, storage, gRPC service
  monitor-service/   check domain, runner, scheduler, storage, gRPC service
  alert-service/     alert domain, lifecycle, storage, gRPC service
  notification-service/
                     planned notification domain, Kafka consumer, email delivery

libs/
  netwatch-proto/    service-specific generated gRPC targets
  target-client/     typed gRPC client for target-service
  monitor-client/    typed gRPC client for monitor-service
  alert-client/      typed gRPC client for alert-service

proto/netwatch/      service contracts
tests/integration/   end-to-end gateway flow
docker/              production-like service image build
scripts/             local verification helpers
```

## Данные

У каждого сервиса отдельная PostgreSQL база:

- `target-service` владеет таблицей `targets`;
- `monitor-service` владеет таблицами `check_results` и `target_check_leases`;
- `alert-service` владеет таблицами `alerts` и `alert_outbox_events`.

Между базами нет shared tables, foreign keys и прямых SQL-запросов из чужого
сервиса. Синхронные связи между доменами проходят через gRPC contracts, а
асинхронные события доставляются через Kafka.

Миграции применяются отдельными Compose jobs:

- `target-migrations`;
- `monitor-migrations`;
- `alert-migrations`.

Каждый job запускает `scripts/apply_migrations.sh`, применяет SQL-файлы из
service-owned `postgresql/migrations` и фиксирует версии в `schema_migrations`.
Сервисы стартуют только после успешного завершения своего migration job, поэтому
этот flow работает и для пустой, и для уже существующей базы.

## Внутренние gRPC contracts

- `proto/netwatch/target_service.proto` - `TargetService`;
- `proto/netwatch/monitor_service.proto` - `CheckService`;
- `proto/netwatch/alert_service.proto` - `AlertService`.

Контракты версионируются через package names:

- `netwatch.target.v1`;
- `netwatch.monitor.v1`;
- `netwatch.alert.v1`.

`alert-service` принимает собственные snapshot messages, чтобы не зависеть от
внутренних моделей target/check сервисов.

## Kafka events

Сейчас используется topic `netwatch.alert.events.v1`.

`alert-service` публикует события:

- `alert.opened`;
- `alert.resolved`.

Payload события содержит `event_id`, `event_type`, `producer`, `occurred_at`,
snapshot alert и snapshot target. Эти события предназначены для
`notification-service`, который будет отправлять email-уведомления независимо
от основного request flow.

## Основной runtime flow

```mermaid
sequenceDiagram
    participant C as Client
    participant G as api-gateway
    participant T as target-service
    participant M as monitor-service
    participant A as alert-service
    participant K as Kafka
    participant N as notification-service

    C->>G: POST /api/v1/targets
    G->>T: CreateTarget gRPC
    T-->>G: Target
    G-->>C: 201 Created

    C->>G: POST /api/v1/targets/{id}/check
    G->>M: RunCheck gRPC
    M->>T: GetTarget gRPC
    T-->>M: Target
    M->>M: HTTP/TCP check + save result
    M->>A: ProcessCheckResult gRPC
    A->>A: open/resolve alert
    A-->>K: publish alert.opened / alert.resolved
    K-->>N: consume alert event
    N-->>N: send email notification
    M-->>G: CheckResult
    G-->>C: 201 Created
```

## Public HTTP API

Локально `api-gateway` доступен на `http://localhost:8081`.

Документация:

- Swagger UI: `http://localhost:8081/docs`;
- OpenAPI JSON: `http://localhost:8081/openapi.json`;
- healthcheck: `http://localhost:8081/ping`.

Основные endpoint'ы:

- `POST /api/v1/targets`;
- `GET /api/v1/targets`;
- `GET /api/v1/targets/{id}`;
- `PATCH /api/v1/targets/{id}`;
- `DELETE /api/v1/targets/{id}`;
- `POST /api/v1/targets/{id}/check`;
- `GET /api/v1/targets/{id}/checks`;
- `GET /api/v1/targets/{id}/status`;
- `GET /api/v1/alerts`;
- `GET /api/v1/alerts/active`.

## Локальные адреса

| Component | Address |
| --- | --- |
| API Gateway | `http://localhost:8081` |
| Swagger UI | `http://localhost:8081/docs` |
| OpenAPI JSON | `http://localhost:8081/openapi.json` |
| Target service healthcheck | `http://localhost:8082/ping` |
| Kafka bootstrap | `localhost:19092` |
| Target PostgreSQL | `localhost:15432` |
| Monitor PostgreSQL | `localhost:15434` |
| Alert PostgreSQL | `localhost:15435` |

Внутренние gRPC endpoints внутри Docker Compose:

- `target-service:8090`;
- `monitor-service:8091`;
- `alert-service:8092`.

## Быстрый запуск

```bash
docker compose up --build
```

После старта можно открыть Swagger UI:

```text
http://localhost:8081/docs
```

## Быстрая проверка

Собрать все сервисы:

```bash
./scripts/quick_check.sh build
```

Запустить end-to-end flow через `api-gateway`:

```bash
./tests/integration/run_api_gateway_flow.py
```

## CI/CD

GitHub Actions workflow собирает сервисы отдельно, запускает unit/integration
checks, собирает production-like Docker images и публикует их в GHCR.

Публикуемые images:

- `ghcr.io/<owner>/<repo>/api-gateway:<sha|branch>`;
- `ghcr.io/<owner>/<repo>/target-service:<sha|branch>`;
- `ghcr.io/<owner>/<repo>/monitor-service:<sha|branch>`;
- `ghcr.io/<owner>/<repo>/alert-service:<sha|branch>`.
