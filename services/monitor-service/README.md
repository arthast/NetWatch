# monitor-service

`monitor-service` is the internal NetWatch service responsible for checks,
check history, alerts, and the scheduler. It exposes internal gRPC APIs for
`api-gateway` and uses `target-service` over gRPC to load targets.

Use the root [README](../../README.md) for Docker Compose demo commands and the
full architecture overview.
