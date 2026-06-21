#!/usr/bin/env bash
set -euo pipefail

: "${NETWATCH_SERVICE:?NETWATCH_SERVICE is required}"

config_dir="/opt/netwatch/etc/${NETWATCH_SERVICE}"
source_vars="${config_dir}/config_vars.compose.yaml"
runtime_vars="/tmp/netwatch-config-vars.yaml"
secdist_path="${config_dir}/secdist.compose.json"

python3 - "${source_vars}" "${runtime_vars}" "${secdist_path}" "${NETWATCH_SERVICE}" <<'PY'
import os
import sys
from pathlib import Path

source, destination, secdist_path, service = sys.argv[1:]

replacements = {
    "secdist-path": secdist_path,
}

if service == "notification_service":
    env_replacements = {
        "notification-email-provider": "NOTIFICATION_EMAIL_PROVIDER",
        "notification-email-provider-url": "NOTIFICATION_EMAIL_PROVIDER_URL",
        "notification-email-from-email": "NOTIFICATION_EMAIL_FROM_EMAIL",
        "notification-email-from-name": "NOTIFICATION_EMAIL_FROM_NAME",
        "notification-email-seed-recipient-email": "NOTIFICATION_EMAIL_SEED_RECIPIENT_EMAIL",
        "notification-yandex-postbox-metadata-token-url": "NOTIFICATION_YANDEX_POSTBOX_METADATA_TOKEN_URL",
    }
    for config_key, env_key in env_replacements.items():
        value = os.getenv(env_key)
        if value:
            replacements[config_key] = value

def quote(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"

lines = []
for line in Path(source).read_text(encoding="utf-8").splitlines():
    key = line.split(":", 1)[0]
    if key in replacements:
        lines.append(f"{key}: {quote(replacements[key])}")
        continue
    lines.append(line)

Path(destination).write_text("\n".join(lines) + "\n", encoding="utf-8")
PY

exec "/opt/netwatch/bin/${NETWATCH_SERVICE}" \
  --config "${config_dir}/static_config.yaml" \
  --config_vars "${runtime_vars}"
