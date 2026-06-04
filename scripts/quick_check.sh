#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${NETWATCH_QUICK_BUILD_DIR:-build-quick-debug}"
BUILD_JOBS="${NETWATCH_BUILD_JOBS:-1}"
PROJECT_NAME="${NETWATCH_QUICK_PROJECT:-netwatch-quick-dev}"

usage() {
  cat <<EOF
Usage: scripts/quick_check.sh <command>

Commands:
  build      Incrementally build all service binaries in Docker
  flow       Run the integration flow and clean Compose resources afterwards
  flow-keep  Run the integration flow and keep Compose services up
  flow-skip  Re-run the integration flow against already running services
  down       Stop and remove the kept quick-check Compose project

Environment:
  NETWATCH_QUICK_BUILD_DIR  CMake build dir, default: build-quick-debug
  NETWATCH_BUILD_JOBS       Ninja jobs, default: 1
  NETWATCH_QUICK_PROJECT    Compose project, default: netwatch-quick-dev
EOF
}

build() {
  docker compose run --rm --no-deps --workdir /workspace monitor-service \
    bash -lc "cmake -S . -B \"$BUILD_DIR\" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DUSERVER_FEATURE_GRPC=ON -DUSERVER_FEATURE_KAFKA=ON -DUSERVER_FEATURE_POSTGRESQL=ON -DUSERVER_SANITIZE='addr;ub' -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build \"$BUILD_DIR\" -j \"$BUILD_JOBS\" --target api_gateway target_service monitor_service alert_service notification_service"
}

case "${1:-}" in
  build)
    build
    ;;
  flow)
    "$ROOT_DIR/tests/integration/run_api_gateway_flow.py" \
      --project-name "$PROJECT_NAME"
    ;;
  flow-keep)
    "$ROOT_DIR/tests/integration/run_api_gateway_flow.py" \
      --project-name "$PROJECT_NAME" \
      --keep-up
    ;;
  flow-skip)
    "$ROOT_DIR/tests/integration/run_api_gateway_flow.py" --skip-compose
    ;;
  down)
    docker compose -p "$PROJECT_NAME" down -v --remove-orphans
    ;;
  -h|--help|help)
    usage
    ;;
  *)
    usage >&2
    exit 2
    ;;
esac
