CREATE TABLE IF NOT EXISTS target_check_leases (
    target_id BIGINT PRIMARY KEY,
    owner_id TEXT NOT NULL,
    locked_until TIMESTAMPTZ NOT NULL,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_target_check_leases_locked_until
    ON target_check_leases(locked_until);
