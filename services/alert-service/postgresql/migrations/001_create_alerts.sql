CREATE TABLE IF NOT EXISTS alerts (
    id BIGSERIAL PRIMARY KEY,
    target_id BIGINT NOT NULL,
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
