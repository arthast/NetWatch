#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "Usage: apply_migrations.sh <service-name> <migrations-dir> <database-url>" >&2
  exit 2
fi

SERVICE_NAME="$1"
MIGRATIONS_DIR="$2"
DATABASE_URL="$3"

if [[ ! -d "$MIGRATIONS_DIR" ]]; then
  echo "Migrations directory does not exist: $MIGRATIONS_DIR" >&2
  exit 2
fi

echo "Waiting for database for $SERVICE_NAME..."
for attempt in $(seq 1 60); do
  if psql "$DATABASE_URL" -v ON_ERROR_STOP=1 -c 'SELECT 1' >/dev/null 2>&1; then
    break
  fi

  if [[ "$attempt" -eq 60 ]]; then
    echo "Database did not become available for $SERVICE_NAME" >&2
    exit 1
  fi

  sleep 1
done

psql "$DATABASE_URL" -v ON_ERROR_STOP=1 <<'SQL'
CREATE TABLE IF NOT EXISTS schema_migrations (
    version TEXT PRIMARY KEY,
    checksum TEXT NOT NULL,
    applied_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
SQL

mapfile -t migrations < <(find "$MIGRATIONS_DIR" -maxdepth 1 -type f -name '*.sql' | sort)

if [[ "${#migrations[@]}" -eq 0 ]]; then
  echo "No migrations found for $SERVICE_NAME in $MIGRATIONS_DIR"
  exit 0
fi

for migration in "${migrations[@]}"; do
  version="$(basename "$migration")"
  checksum="$(sha256sum "$migration" | awk '{print $1}')"
  applied_checksum="$(
    psql "$DATABASE_URL" -v ON_ERROR_STOP=1 -At -v version="$version" <<'SQL'
SELECT checksum FROM schema_migrations WHERE version = :'version';
SQL
  )"

  if [[ -n "$applied_checksum" ]]; then
    if [[ "$applied_checksum" != "$checksum" ]]; then
      echo "Checksum mismatch for $SERVICE_NAME migration $version" >&2
      echo "Applied: $applied_checksum" >&2
      echo "Current: $checksum" >&2
      exit 1
    fi

    echo "Skipping already applied $SERVICE_NAME migration $version"
    continue
  fi

  echo "Applying $SERVICE_NAME migration $version"
  psql "$DATABASE_URL" -v ON_ERROR_STOP=1 \
    -v version="$version" \
    -v checksum="$checksum" <<SQL
BEGIN;
\\i $migration
INSERT INTO schema_migrations (version, checksum)
VALUES (:'version', :'checksum');
COMMIT;
SQL
done

echo "Migrations are up to date for $SERVICE_NAME"
