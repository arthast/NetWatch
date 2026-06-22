UPDATE notification_recipients
SET
    is_enabled = FALSE,
    updated_at = NOW()
WHERE is_enabled = TRUE;
