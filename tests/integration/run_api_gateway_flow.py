#!/usr/bin/env python3
import argparse
import json
import re
import subprocess
import sys
import time
import urllib.error
import urllib.request
from dataclasses import dataclass
from typing import Any


DEFAULT_BASE_URL = "http://localhost:8081"
MAILPIT_BASE_URL = "http://localhost:8025"
VERIFICATION_TOKEN_RE = re.compile(r"verify-email\?token=([0-9a-fA-F]{64})")


@dataclass(frozen=True)
class HttpResponse:
    status: int
    body: str

    def json(self) -> Any:
        return json.loads(self.body)


def run(command: list[str], *, cwd: str | None = None) -> None:
    subprocess.run(command, cwd=cwd, check=True)


def run_capture(command: list[str], *, cwd: str | None = None) -> str:
    result = subprocess.run(
        command,
        cwd=cwd,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    return result.stdout


def dump_compose_diagnostics(compose: list[str]) -> None:
    interesting_services = [
        "api-gateway",
        "frontend",
        "monitor-service",
        "target-service",
        "alert-service",
        "notification-service",
        "auth-service",
    ]
    try:
        available_services = set(
            run_capture([*compose, "config", "--services"]).splitlines()
        )
    except subprocess.CalledProcessError:
        available_services = set(interesting_services)

    log_services = [
        service for service in interesting_services if service in available_services
    ]
    commands = [
        [*compose, "ps", "-a"],
    ]
    if log_services:
        commands.append(
            [
                *compose,
                "logs",
                "--no-color",
                "--tail",
                "200",
                *log_services,
            ]
        )

    for command in commands:
        print(f"+ {' '.join(command)}", file=sys.stderr)
        subprocess.run(command, check=False)


def request(
    method: str,
    path: str,
    *,
    base_url: str,
    payload: dict[str, Any] | None = None,
    headers: dict[str, str] | None = None,
) -> HttpResponse:
    body = None
    request_headers = dict(headers or {})
    if payload is not None:
        body = json.dumps(payload).encode("utf-8")
        request_headers["Content-Type"] = "application/json"

    request = urllib.request.Request(
        base_url + path,
        data=body,
        headers=request_headers,
        method=method,
    )

    try:
        with urllib.request.urlopen(request, timeout=10) as response:
            return HttpResponse(
                status=response.status,
                body=response.read().decode("utf-8"),
            )
    except urllib.error.HTTPError as error:
        return HttpResponse(
            status=error.code,
            body=error.read().decode("utf-8"),
        )


def get_json(url: str) -> Any:
    with urllib.request.urlopen(url, timeout=10) as response:
        return json.loads(response.read().decode("utf-8"))


def wait_for_gateway(base_url: str, timeout_seconds: int) -> None:
    deadline = time.monotonic() + timeout_seconds
    last_error: Exception | None = None

    while time.monotonic() < deadline:
        try:
            response = request("GET", "/ping", base_url=base_url)
            if response.status == 200:
                return
        except Exception as error:  # noqa: BLE001 - include connection errors
            last_error = error
        time.sleep(1)

    message = f"gateway did not become healthy within {timeout_seconds}s"
    if last_error is not None:
        message += f": {last_error}"
    raise RuntimeError(message)


def assert_status(response: HttpResponse, expected: int) -> None:
    if response.status != expected:
        raise AssertionError(
            f"expected HTTP {expected}, got {response.status}: {response.body}"
        )


def wait_for_verification_token(email: str, timeout_seconds: int = 30) -> str:
    deadline = time.monotonic() + timeout_seconds
    last_payload: Any = None

    while time.monotonic() < deadline:
        payload = get_json(f"{MAILPIT_BASE_URL}/api/v1/messages")
        last_payload = payload
        messages = payload.get("messages", [])
        for message in messages:
            subject = message.get("Subject") or message.get("subject") or ""
            if "Verify your email" not in subject:
                continue

            raw_to = message.get("To") or message.get("to") or []
            to_text = json.dumps(raw_to)
            if email not in to_text:
                continue

            message_text = json.dumps(message)
            match = VERIFICATION_TOKEN_RE.search(message_text)
            if match:
                return match.group(1)

            message_id = message.get("ID") or message.get("id")
            if not message_id:
                continue
            detail = get_json(f"{MAILPIT_BASE_URL}/api/v1/message/{message_id}")
            detail_text = json.dumps(detail)
            match = VERIFICATION_TOKEN_RE.search(detail_text)
            if match:
                return match.group(1)
        time.sleep(1)

    raise RuntimeError(
        "verification email did not appear in Mailpit. "
        f"Last messages payload:\n{json.dumps(last_payload, indent=2)}"
    )


def test_auth_flow(base_url: str) -> tuple[dict[str, str], str]:
    suffix = int(time.time() * 1000)
    credentials = {
        "email": f"integration-{suffix}@example.test",
        "password": "password123",
    }

    missing_email = request(
        "POST",
        "/api/v1/auth/register",
        base_url=base_url,
        payload={"password": "password123"},
    )
    assert_status(missing_email, 400)

    register = request(
        "POST",
        "/api/v1/auth/register",
        base_url=base_url,
        payload=credentials,
    )
    assert_status(register, 201)
    auth = register.json()
    assert auth["user"]["id"] > 0
    assert auth["user"]["email"] == credentials["email"]
    assert auth["user"]["email_verified"] is False
    assert len(auth["access_token"]) >= 32
    assert auth["expires_at"]

    duplicate = request(
        "POST",
        "/api/v1/auth/register",
        base_url=base_url,
        payload=credentials,
    )
    assert_status(duplicate, 400)

    invalid_login = request(
        "POST",
        "/api/v1/auth/login",
        base_url=base_url,
        payload={**credentials, "password": "wrong-password"},
    )
    assert_status(invalid_login, 401)

    login = request(
        "POST",
        "/api/v1/auth/login",
        base_url=base_url,
        payload=credentials,
    )
    assert_status(login, 200)
    login_body = login.json()
    assert login_body["user"] == auth["user"]
    assert len(login_body["access_token"]) >= 32

    missing_token = request("GET", "/api/v1/auth/me", base_url=base_url)
    assert_status(missing_token, 401)

    me = request(
        "GET",
        "/api/v1/auth/me",
        base_url=base_url,
        headers={"Authorization": f"Bearer {login_body['access_token']}"},
    )
    assert_status(me, 200)
    session = me.json()
    assert session["user"] == auth["user"]
    assert session["expires_at"] == login_body["expires_at"]

    unverified_recipient = request(
        "POST",
        "/api/v1/notifications/recipients",
        base_url=base_url,
        headers={"Authorization": f"Bearer {login_body['access_token']}"},
        payload={"email": credentials["email"]},
    )
    assert_status(unverified_recipient, 403)

    token = wait_for_verification_token(credentials["email"])
    verify = request(
        "POST",
        "/api/v1/auth/verify-email",
        base_url=base_url,
        payload={"token": token},
    )
    assert_status(verify, 200)
    verified_session = verify.json()
    assert verified_session["user"]["email"] == credentials["email"]
    assert verified_session["user"]["email_verified"] is True

    me_after_verify = request(
        "GET",
        "/api/v1/auth/me",
        base_url=base_url,
        headers={"Authorization": f"Bearer {login_body['access_token']}"},
    )
    assert_status(me_after_verify, 200)
    assert me_after_verify.json()["user"]["email_verified"] is True

    return {"Authorization": f"Bearer {login_body['access_token']}"}, credentials["email"]


def test_target_crud(base_url: str, auth_headers: dict[str, str]) -> int:
    anonymous_list = request("GET", "/api/v1/targets", base_url=base_url)
    assert_status(anonymous_list, 401)

    invalid = request(
        "POST",
        "/api/v1/targets",
        base_url=base_url,
        headers=auth_headers,
        payload={
            "name": "Invalid HTTP target",
            "type": "http",
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )
    assert_status(invalid, 400)

    create = request(
        "POST",
        "/api/v1/targets",
        base_url=base_url,
        headers=auth_headers,
        payload={
            "name": "Gateway integration HTTP target",
            "type": "http",
            "url": "http://localhost:8080/ping",
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )
    assert_status(create, 201)
    target = create.json()
    assert target["id"] > 0
    assert target["method"] == "GET"
    assert target["expected_status_code"] == 200

    get = request(
        "GET",
        f"/api/v1/targets/{target['id']}",
        base_url=base_url,
        headers=auth_headers,
    )
    assert_status(get, 200)
    assert get.json() == target

    missing = request(
        "GET",
        "/api/v1/targets/999999",
        base_url=base_url,
        headers=auth_headers,
    )
    assert_status(missing, 404)

    patch = request(
        "PATCH",
        f"/api/v1/targets/{target['id']}",
        base_url=base_url,
        headers=auth_headers,
        payload={"name": "Gateway integration HTTP target v2"},
    )
    assert_status(patch, 200)
    assert patch.json()["name"] == "Gateway integration HTTP target v2"

    list_response = request(
        "GET", "/api/v1/targets", base_url=base_url, headers=auth_headers
    )
    assert_status(list_response, 200)
    assert any(item["id"] == target["id"] for item in list_response.json())
    return int(target["id"])


def test_target_crud_edges(base_url: str, auth_headers: dict[str, str]) -> None:
    invalid_id = request(
        "GET",
        "/api/v1/targets/not-a-number",
        base_url=base_url,
        headers=auth_headers,
    )
    assert_status(invalid_id, 400)

    invalid_tcp = request(
        "POST",
        "/api/v1/targets",
        base_url=base_url,
        headers=auth_headers,
        payload={
            "name": "Broken TCP target",
            "type": "tcp",
            "host": "localhost",
            "port": 70000,
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )
    assert_status(invalid_tcp, 400)

    tcp_create = request(
        "POST",
        "/api/v1/targets",
        base_url=base_url,
        headers=auth_headers,
        payload={
            "name": "Gateway integration TCP target",
            "type": "tcp",
            "host": "localhost",
            "port": 5432,
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )
    assert_status(tcp_create, 201)
    tcp_target = tcp_create.json()
    assert tcp_target["type"] == "tcp"
    assert tcp_target["host"] == "localhost"
    assert tcp_target["port"] == 5432

    empty_patch = request(
        "PATCH",
        f"/api/v1/targets/{tcp_target['id']}",
        base_url=base_url,
        headers=auth_headers,
        payload={},
    )
    assert_status(empty_patch, 400)

    protocol_patch = request(
        "PATCH",
        f"/api/v1/targets/{tcp_target['id']}",
        base_url=base_url,
        headers=auth_headers,
        payload={
            "type": "http",
            "url": "http://localhost:8080/ping",
            "method": "HEAD",
            "expected_status_code": 200,
        },
    )
    assert_status(protocol_patch, 200)
    patched = protocol_patch.json()
    assert patched["type"] == "http"
    assert patched["url"] == "http://localhost:8080/ping"
    assert patched["method"] == "HEAD"

    delete_response = request(
        "DELETE",
        f"/api/v1/targets/{tcp_target['id']}",
        base_url=base_url,
        headers=auth_headers,
    )
    assert_status(delete_response, 204)

    deleted_get = request(
        "GET",
        f"/api/v1/targets/{tcp_target['id']}",
        base_url=base_url,
        headers=auth_headers,
    )
    assert_status(deleted_get, 404)


def test_checks(
    base_url: str, target_id: int, auth_headers: dict[str, str]
) -> None:
    check_response = request(
        "POST",
        f"/api/v1/targets/{target_id}/check",
        base_url=base_url,
        headers=auth_headers,
    )
    assert_status(check_response, 201)
    check = check_response.json()
    assert check["target_id"] == target_id
    assert check["status"] == "up"
    assert check["protocol"] == "http"

    status_response = request(
        "GET",
        f"/api/v1/targets/{target_id}/status",
        base_url=base_url,
        headers=auth_headers,
    )
    assert_status(status_response, 200)
    status = status_response.json()
    assert status["target_id"] == target_id
    assert status["status"] == "up"
    assert status["protocol"] == "http"

    history_response = request(
        "GET",
        f"/api/v1/targets/{target_id}/checks",
        base_url=base_url,
        headers=auth_headers,
    )
    assert_status(history_response, 200)
    assert any(item["id"] == check["id"] for item in history_response.json())


def test_notification_recipient(
    base_url: str, auth_headers: dict[str, str], auth_email: str
) -> int:
    anonymous_list = request(
        "GET",
        "/api/v1/notifications/recipients",
        base_url=base_url,
    )
    assert_status(anonymous_list, 401)

    create = request(
        "POST",
        "/api/v1/notifications/recipients",
        base_url=base_url,
        headers=auth_headers,
        payload={"email": "attacker@example.test"},
    )
    assert_status(create, 201)
    recipient = create.json()
    assert recipient["id"] > 0
    assert recipient["email"] == auth_email
    assert recipient["is_enabled"] is True

    update_email = request(
        "PATCH",
        f"/api/v1/notifications/recipients/{recipient['id']}",
        base_url=base_url,
        headers=auth_headers,
        payload={"email": "attacker@example.test"},
    )
    assert_status(update_email, 400)

    list_response = request(
        "GET",
        "/api/v1/notifications/recipients",
        base_url=base_url,
        headers=auth_headers,
    )
    assert_status(list_response, 200)
    assert any(item["id"] == recipient["id"] for item in list_response.json())
    return int(recipient["id"])


def test_alert_lifecycle(base_url: str, auth_headers: dict[str, str]) -> None:
    create = request(
        "POST",
        "/api/v1/targets",
        base_url=base_url,
        headers=auth_headers,
        payload={
            "name": "Gateway integration broken TCP target",
            "type": "tcp",
            "host": "localhost",
            "port": 1,
            "interval_seconds": 30,
            "timeout_ms": 500,
        },
    )
    assert_status(create, 201)
    target = create.json()
    target_id = target["id"]

    anonymous_settings = request(
        "GET",
        f"/api/v1/targets/{target_id}/notifications",
        base_url=base_url,
    )
    assert_status(anonymous_settings, 401)

    settings = request(
        "GET",
        f"/api/v1/targets/{target_id}/notifications",
        base_url=base_url,
        headers=auth_headers,
    )
    assert_status(settings, 200)
    assert settings.json()["email_enabled"] is True

    update_settings = request(
        "PATCH",
        f"/api/v1/targets/{target_id}/notifications",
        base_url=base_url,
        headers=auth_headers,
        payload={"email_enabled": True},
    )
    assert_status(update_settings, 200)
    assert update_settings.json()["email_enabled"] is True

    down_check = request(
        "POST",
        f"/api/v1/targets/{target_id}/check",
        base_url=base_url,
        headers=auth_headers,
    )
    assert_status(down_check, 201)
    assert down_check.json()["status"] == "down"

    active_alerts = request(
        "GET", "/api/v1/alerts/active", base_url=base_url, headers=auth_headers
    )
    assert_status(active_alerts, 200)
    assert any(
        alert["target_id"] == target_id and alert["type"] == "target_down"
        for alert in active_alerts.json()
    )

    patch = request(
        "PATCH",
        f"/api/v1/targets/{target_id}",
        base_url=base_url,
        headers=auth_headers,
        payload={"port": 8080},
    )
    assert_status(patch, 200)

    up_check = request(
        "POST",
        f"/api/v1/targets/{target_id}/check",
        base_url=base_url,
        headers=auth_headers,
    )
    assert_status(up_check, 201)
    assert up_check.json()["status"] == "up"

    active_after_recovery = request(
        "GET",
        "/api/v1/alerts/active",
        base_url=base_url,
        headers=auth_headers,
    )
    assert_status(active_after_recovery, 200)
    assert not any(
        alert["target_id"] == target_id and alert["type"] == "target_down"
        for alert in active_after_recovery.json()
    )


def run_flow(base_url: str) -> None:
    docs = request("GET", "/docs", base_url=base_url)
    assert_status(docs, 200)
    assert "SwaggerUIBundle" in docs.body
    assert "/openapi.json" in docs.body

    openapi = request("GET", "/openapi.json", base_url=base_url)
    assert_status(openapi, 200)
    spec = openapi.json()
    assert spec["openapi"] == "3.0.3"
    assert spec["info"]["title"] == "NetWatch API"
    assert "/api/v1/auth/register" in spec["paths"]
    assert "/api/v1/auth/login" in spec["paths"]
    assert "/api/v1/auth/me" in spec["paths"]
    assert "/api/v1/auth/verify-email" in spec["paths"]
    assert "/api/v1/auth/resend-verification-email" in spec["paths"]
    assert "/api/v1/targets" in spec["paths"]
    assert "/api/v1/targets/{id}/notifications" in spec["paths"]
    assert "/api/v1/alerts/active" in spec["paths"]

    auth_headers, auth_email = test_auth_flow(base_url)
    target_id = test_target_crud(base_url, auth_headers)
    test_target_crud_edges(base_url, auth_headers)
    test_checks(base_url, target_id, auth_headers)
    test_notification_recipient(base_url, auth_headers, auth_email)
    test_alert_lifecycle(base_url, auth_headers)


def wait_for_alert_events_in_kafka(
    compose: list[str], timeout_seconds: int = 30
) -> None:
    deadline = time.monotonic() + timeout_seconds
    last_output = ""

    while time.monotonic() < deadline:
        output = run_capture(
            [
                *compose,
                "exec",
                "-T",
                "kafka",
                "bash",
                "-lc",
                (
                    "/opt/kafka/bin/kafka-console-consumer.sh "
                    "--bootstrap-server localhost:9092 "
                    "--topic netwatch.alert.events.v1 "
                    "--from-beginning "
                    "--timeout-ms 5000 "
                    "--max-messages 2"
                ),
            ]
        )
        last_output = output
        messages = [
            json.loads(line)
            for line in output.splitlines()
            if line.strip().startswith("{")
        ]
        event_types = {message.get("event_type") for message in messages}
        if {"alert.opened", "alert.resolved"} <= event_types:
            return
        time.sleep(1)

    raise RuntimeError(
        "Kafka alert events did not appear in netwatch.alert.events.v1. "
        f"Last consumer output:\n{last_output}"
    )


def wait_for_notification_events(
    compose: list[str], timeout_seconds: int = 30
) -> None:
    deadline = time.monotonic() + timeout_seconds
    last_output = ""

    while time.monotonic() < deadline:
        output = run_capture(
            [
                *compose,
                "exec",
                "-T",
                "notification-postgres",
                "psql",
                "-U",
                "notification_service",
                "-d",
                "notification_service_db",
                "-Atc",
                (
                    "SELECT event_type || ':' || status "
                    "FROM notification_events "
                    "JOIN notification_deliveries USING (event_id) "
                    "ORDER BY event_type, status"
                ),
            ]
        )
        last_output = output
        rows = {line.strip() for line in output.splitlines() if line.strip()}
        if {"alert.opened:sent", "alert.resolved:sent"} <= rows:
            return
        time.sleep(1)

    raise RuntimeError(
        "notification-service did not persist alert notification events. "
        f"Last query output:\n{last_output}"
    )


def wait_for_mailpit_messages(compose: list[str], timeout_seconds: int = 30) -> None:
    deadline = time.monotonic() + timeout_seconds
    last_output = ""

    while time.monotonic() < deadline:
        output = run_capture(
            [
                *compose,
                "exec",
                "-T",
                "api-gateway",
                "curl",
                "-fsS",
                "http://mailpit:8025/api/v1/messages",
            ]
        )
        last_output = output
        payload = json.loads(output)
        subjects = {
            message.get("Subject", "")
            for message in payload.get("messages", [])
        }
        if any("Target is down" in subject for subject in subjects) and any(
            "Target recovered" in subject for subject in subjects
        ):
            return
        time.sleep(1)

    raise RuntimeError(
        "Mailpit did not receive alert notification emails. "
        f"Last response:\n{last_output}"
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run api-gateway integration tests against Docker Compose."
    )
    parser.add_argument("--base-url", default=DEFAULT_BASE_URL)
    parser.add_argument("--project-name", default=f"netwatch-it-{int(time.time())}")
    parser.add_argument("--no-build", action="store_true")
    parser.add_argument("--keep-up", action="store_true")
    parser.add_argument("--skip-compose", action="store_true")
    parser.add_argument(
        "-f",
        "--compose-file",
        action="append",
        default=None,
        help="Compose file to use. May be passed multiple times.",
    )
    parser.add_argument("--timeout-seconds", type=int, default=180)
    args = parser.parse_args()

    compose = ["docker", "compose"]
    for compose_file in args.compose_file or ["docker-compose.yml"]:
        compose.extend(["-f", compose_file])
    compose.extend(["-p", args.project_name])

    try:
        if not args.skip_compose:
            up_command = [*compose, "up", "-d"]
            if not args.no_build:
                up_command.append("--build")
            else:
                up_command.append("--no-build")
            run(up_command)

        wait_for_gateway(args.base_url, args.timeout_seconds)
        run_flow(args.base_url)
        if not args.skip_compose:
            wait_for_alert_events_in_kafka(compose)
            wait_for_notification_events(compose)
            wait_for_mailpit_messages(compose)
        print("api-gateway integration flow passed")
        return 0
    except Exception:
        if not args.skip_compose:
            dump_compose_diagnostics(compose)
        raise
    finally:
        if not args.skip_compose and not args.keep_up:
            subprocess.run([*compose, "down", "-v", "--remove-orphans"], check=False)


if __name__ == "__main__":
    sys.exit(main())
