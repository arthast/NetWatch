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

CREATE TABLE IF NOT EXISTS check_results (
    id BIGSERIAL PRIMARY KEY,
    target_id BIGINT NOT NULL REFERENCES targets(id),
    status TEXT NOT NULL CHECK (status IN ('up', 'down')),
    protocol TEXT NOT NULL CHECK (protocol IN ('http', 'tcp')),

    http_status INTEGER,
    latency_ms INTEGER,
    error_message TEXT,

    checked_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_check_results_target_checked_at
    ON check_results(target_id, checked_at DESC);

CREATE TABLE IF NOT EXISTS alerts (
    id BIGSERIAL PRIMARY KEY,
    target_id BIGINT NOT NULL REFERENCES targets(id),
    type TEXT NOT NULL CHECK (type IN ('target_down', 'target_recovered', 'high_latency')),
    severity TEXT NOT NULL CHECK (severity IN ('warning', 'critical')),
    message TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    resolved_at TIMESTAMPTZ
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_alerts_one_active_target_down
    ON alerts(target_id, type)
    WHERE type = 'target_down' AND resolved_at IS NULL;

CREATE INDEX IF NOT EXISTS idx_alerts_active_created_at
    ON alerts(created_at DESC, id DESC)
    WHERE resolved_at IS NULL;
