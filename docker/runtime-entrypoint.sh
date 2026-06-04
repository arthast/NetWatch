#!/usr/bin/env bash
set -euo pipefail

: "${NETWATCH_SERVICE:?NETWATCH_SERVICE is required}"

config_dir="/opt/netwatch/etc/${NETWATCH_SERVICE}"
source_vars="${config_dir}/config_vars.compose.yaml"
runtime_vars="/tmp/netwatch-config-vars.yaml"
secdist_path="${config_dir}/secdist.compose.json"

python3 - "${source_vars}" "${runtime_vars}" "${secdist_path}" <<'PY'
import sys
from pathlib import Path

source, destination, secdist_path = sys.argv[1:]

def quote(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"

lines = []
for line in Path(source).read_text(encoding="utf-8").splitlines():
    if line.startswith("secdist-path:"):
        lines.append(f"secdist-path: {quote(secdist_path)}")
        continue
    lines.append(line)

Path(destination).write_text("\n".join(lines) + "\n", encoding="utf-8")
PY

exec "/opt/netwatch/bin/${NETWATCH_SERVICE}" \
  --config "${config_dir}/static_config.yaml" \
  --config_vars "${runtime_vars}"
