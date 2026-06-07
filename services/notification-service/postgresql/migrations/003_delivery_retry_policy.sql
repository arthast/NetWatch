ALTER TABLE notification_deliveries
    ADD COLUMN IF NOT EXISTS next_retry_at TIMESTAMPTZ;

ALTER TABLE notification_deliveries
    DROP CONSTRAINT IF EXISTS notification_deliveries_status_check;

ALTER TABLE notification_deliveries
    ADD CONSTRAINT notification_deliveries_status_check
    CHECK (status IN ('pending', 'sending', 'retry_scheduled', 'sent', 'skipped', 'failed'));

CREATE INDEX IF NOT EXISTS idx_notification_deliveries_retry
    ON notification_deliveries(status, next_retry_at, created_at, id)
    WHERE status IN ('pending', 'retry_scheduled', 'sending');
