CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE IF NOT EXISTS alert_outbox_events (
    event_id TEXT PRIMARY KEY DEFAULT gen_random_uuid()::TEXT,
    event_type TEXT NOT NULL CHECK (event_type IN ('alert.opened', 'alert.resolved')),
    aggregate_type TEXT NOT NULL DEFAULT 'alert',
    aggregate_id BIGINT NOT NULL,
    partition_key TEXT NOT NULL,
    payload JSONB NOT NULL,
    status TEXT NOT NULL DEFAULT 'pending'
        CHECK (status IN ('pending', 'publishing', 'published', 'failed')),
    attempts INT NOT NULL DEFAULT 0 CHECK (attempts >= 0),
    next_retry_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    last_error TEXT,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    published_at TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS idx_alert_outbox_events_pending
    ON alert_outbox_events(status, next_retry_at, created_at)
    WHERE status IN ('pending', 'failed');

CREATE INDEX IF NOT EXISTS idx_alert_outbox_events_aggregate
    ON alert_outbox_events(aggregate_type, aggregate_id, created_at);
