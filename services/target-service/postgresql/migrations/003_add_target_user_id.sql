ALTER TABLE targets
    ADD COLUMN IF NOT EXISTS user_id BIGINT;

DROP INDEX IF EXISTS idx_targets_unique_active_equivalent;

CREATE UNIQUE INDEX IF NOT EXISTS idx_targets_unique_active_equivalent
    ON targets (
        user_id,
        name,
        type,
        url,
        method,
        expected_status_code,
        host,
        port,
        interval_seconds,
        timeout_ms
    )
    NULLS NOT DISTINCT
    WHERE is_active = TRUE;

CREATE INDEX IF NOT EXISTS idx_targets_user_active
    ON targets(user_id, is_active, id)
    WHERE is_active = TRUE;
