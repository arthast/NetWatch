# monitor-service

`monitor-service` is the internal NetWatch service responsible for checks,
check history, and the scheduler. It exposes internal check gRPC APIs for
`api-gateway`, uses `target-service` over gRPC to load targets, and sends check
result transitions to `alert-service`.

Use the root [README](../../README.md) for Docker Compose demo commands and the
full architecture overview.
