ALTER TABLE auth_users
    ADD COLUMN IF NOT EXISTS email_verified_at TIMESTAMPTZ;

CREATE TABLE IF NOT EXISTS auth_email_verification_tokens (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES auth_users(id) ON DELETE CASCADE,
    token_hash TEXT NOT NULL UNIQUE,
    expires_at TIMESTAMPTZ NOT NULL,
    used_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_auth_email_verification_tokens_user_id
    ON auth_email_verification_tokens(user_id, created_at DESC);

CREATE INDEX IF NOT EXISTS idx_auth_email_verification_tokens_expires_at
    ON auth_email_verification_tokens(expires_at)
    WHERE used_at IS NULL;
