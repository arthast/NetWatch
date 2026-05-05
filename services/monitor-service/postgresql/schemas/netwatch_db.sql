CREATE TABLE IF NOT EXISTS targets (
    id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    type TEXT NOT NULL CHECK (type IN ('http', 'tcp')),

    url TEXT,
    method TEXT,
    expected_status_code INTEGER,

    host TEXT,
    port INTEGER,

    interval_seconds INTEGER NOT NULL,
    timeout_ms INTEGER NOT NULL,

    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
