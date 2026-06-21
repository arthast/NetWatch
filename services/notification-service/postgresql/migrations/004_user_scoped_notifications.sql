ALTER TABLE notification_recipients
    ADD COLUMN IF NOT EXISTS user_id BIGINT;

ALTER TABLE notification_recipients
    DROP CONSTRAINT IF EXISTS notification_recipients_email_key;

ALTER TABLE notification_deliveries
    ADD COLUMN IF NOT EXISTS user_id BIGINT,
    ADD COLUMN IF NOT EXISTS target_id BIGINT;

ALTER TABLE notification_deliveries
    DROP CONSTRAINT IF EXISTS notification_deliveries_event_id_recipient_email_key;

CREATE UNIQUE INDEX IF NOT EXISTS idx_notification_recipients_user_email
    ON notification_recipients(user_id, email)
    NULLS NOT DISTINCT;

CREATE UNIQUE INDEX IF NOT EXISTS idx_notification_deliveries_event_recipient_user
    ON notification_deliveries(event_id, recipient_email, user_id)
    NULLS NOT DISTINCT;

CREATE INDEX IF NOT EXISTS idx_notification_deliveries_user_created_at
    ON notification_deliveries(user_id, created_at DESC, id DESC);

CREATE TABLE IF NOT EXISTS notification_target_settings (
    user_id BIGINT NOT NULL,
    target_id BIGINT NOT NULL,
    email_enabled BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    PRIMARY KEY (user_id, target_id)
);
