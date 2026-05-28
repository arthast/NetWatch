CREATE TABLE IF NOT EXISTS check_results (
    id BIGSERIAL PRIMARY KEY,
    target_id BIGINT NOT NULL,
    status TEXT NOT NULL CHECK (status IN ('up', 'down')),
    protocol TEXT NOT NULL CHECK (protocol IN ('http', 'tcp')),

    http_status INTEGER,
    latency_ms INTEGER,
    error_message TEXT,

    checked_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_check_results_target_checked_at
    ON check_results(target_id, checked_at DESC);
