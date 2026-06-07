CREATE TABLE IF NOT EXISTS notification_recipients (
    id BIGSERIAL PRIMARY KEY,
    email TEXT NOT NULL UNIQUE,
    is_enabled BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS notification_events (
    event_id TEXT PRIMARY KEY,
    event_type TEXT NOT NULL CHECK (event_type IN ('alert.opened', 'alert.resolved')),
    payload JSONB NOT NULL,
    received_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    processed_at TIMESTAMPTZ
);

CREATE TABLE IF NOT EXISTS notification_deliveries (
    id BIGSERIAL PRIMARY KEY,
    event_id TEXT NOT NULL REFERENCES notification_events(event_id) ON DELETE CASCADE,
    recipient_id BIGINT REFERENCES notification_recipients(id) ON DELETE SET NULL,
    recipient_email TEXT,
    channel TEXT NOT NULL DEFAULT 'email' CHECK (channel IN ('email')),
    status TEXT NOT NULL CHECK (status IN ('pending', 'sent', 'skipped', 'failed')),
    payload JSONB NOT NULL,
    attempts INT NOT NULL DEFAULT 0 CHECK (attempts >= 0),
    error_message TEXT,
    next_retry_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    delivered_at TIMESTAMPTZ,
    UNIQUE (event_id, recipient_email)
);

CREATE INDEX IF NOT EXISTS idx_notification_recipients_enabled
    ON notification_recipients(is_enabled, id)
    WHERE is_enabled = TRUE;

CREATE INDEX IF NOT EXISTS idx_notification_deliveries_status
    ON notification_deliveries(status, created_at, id);

CREATE INDEX IF NOT EXISTS idx_notification_deliveries_retry
    ON notification_deliveries(status, next_retry_at, created_at, id)
    WHERE status IN ('pending', 'retry_scheduled', 'sending');
