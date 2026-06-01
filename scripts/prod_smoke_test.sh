#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${NETWATCH_BASE_URL:-http://localhost}"
BASE_URL="${BASE_URL%/}"

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required command: $1" >&2
    exit 2
  fi
}

require_command curl
require_command python3

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

HTTP_STATUS_FILE="$TMP_DIR/status"
HTTP_BODY_FILE="$TMP_DIR/body"

request() {
  local method="$1"
  local path="$2"
  local expected_status="$3"
  local payload="${4:-}"

  local curl_args=(
    --silent
    --show-error
    --location
    --request "$method"
    --output "$HTTP_BODY_FILE"
    --write-out "%{http_code}"
    --max-time 15
  )

  if [[ -n "$payload" ]]; then
    curl_args+=(--header "Content-Type: application/json" --data "$payload")
  fi

  local status
  status="$(curl "${curl_args[@]}" "$BASE_URL$path")"
  printf '%s' "$status" >"$HTTP_STATUS_FILE"

  if [[ "$status" != "$expected_status" ]]; then
    echo "Expected $method $path to return HTTP $expected_status, got $status" >&2
    echo "Response body:" >&2
    sed -n '1,120p' "$HTTP_BODY_FILE" >&2
    exit 1
  fi
}

json_value() {
  local expression="$1"
  python3 -c "import json, sys; data=json.load(sys.stdin); print($expression)" <"$HTTP_BODY_FILE"
}

step() {
  echo "==> $1"
}

step "Checking /ping"
request GET /ping 200

step "Checking /openapi.json"
request GET /openapi.json 200
python3 - "$HTTP_BODY_FILE" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as body:
    spec = json.load(body)

assert spec["openapi"] == "3.0.3"
assert "/api/v1/targets" in spec["paths"]
assert "/api/v1/alerts/active" in spec["paths"]
PY

step "Creating HTTP target"
request POST /api/v1/targets 201 '{
  "name": "Production smoke HTTP target",
  "type": "http",
  "url": "http://localhost:8080/ping",
  "interval_seconds": 30,
  "timeout_ms": 1000
}'
TARGET_ID="$(json_value 'data["id"]')"

step "Running manual check for target ${TARGET_ID}"
request POST "/api/v1/targets/${TARGET_ID}/check" 201
CHECK_STATUS="$(json_value 'data["status"]')"
if [[ "$CHECK_STATUS" != "up" ]]; then
  echo "Expected manual check status to be up, got: $CHECK_STATUS" >&2
  exit 1
fi

step "Checking target status"
request GET "/api/v1/targets/${TARGET_ID}/status" 200
STATUS_TARGET_ID="$(json_value 'data["target_id"]')"
if [[ "$STATUS_TARGET_ID" != "$TARGET_ID" ]]; then
  echo "Expected status target_id $TARGET_ID, got: $STATUS_TARGET_ID" >&2
  exit 1
fi

step "Checking active alerts endpoint"
request GET /api/v1/alerts/active 200
python3 - "$HTTP_BODY_FILE" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as body:
    alerts = json.load(body)

assert isinstance(alerts, list)
PY

echo "Production smoke test passed for $BASE_URL"
