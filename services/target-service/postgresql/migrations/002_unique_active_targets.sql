CREATE UNIQUE INDEX IF NOT EXISTS idx_targets_unique_active_equivalent
    ON targets (
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
