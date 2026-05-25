import asyncio


async def create_http_target(service_client, url, expected_status=200):
    response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "HTTP target",
            "type": "http",
            "url": url,
            "expected_status_code": expected_status,
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )
    assert response.status == 201
    return response.json()


async def create_tcp_target(service_client, host, port):
    response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "TCP target",
            "type": "tcp",
            "host": host,
            "port": port,
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )
    assert response.status == 201
    return response.json()


async def test_manual_http_check_up(service_client, mockserver):
    @mockserver.handler("/health")
    async def health(request):
        return mockserver.make_response("OK", status=200)

    target = await create_http_target(service_client, mockserver.url("/health"))

    response = await service_client.post(f"/api/v1/targets/{target['id']}/check")

    assert response.status == 201
    check = response.json()
    assert check["target_id"] == target["id"]
    assert check["status"] == "up"
    assert check["protocol"] == "http"
    assert check["http_status"] == 200
    assert check["latency_ms"] >= 0
    assert "error_message" not in check
    assert "checked_at" in check


async def test_manual_http_check_down_on_unexpected_status(service_client, mockserver):
    @mockserver.handler("/health")
    async def health(request):
        return mockserver.make_response("broken", status=503)

    target = await create_http_target(service_client, mockserver.url("/health"))

    response = await service_client.post(f"/api/v1/targets/{target['id']}/check")

    assert response.status == 201
    check = response.json()
    assert check["status"] == "down"
    assert check["protocol"] == "http"
    assert check["http_status"] == 503
    assert check["error_message"] == "unexpected HTTP status: 503"


async def test_checks_history_and_status(service_client, mockserver):
    @mockserver.handler("/health")
    async def health(request):
        return mockserver.make_response("OK", status=200)

    target = await create_http_target(service_client, mockserver.url("/health"))

    first = await service_client.post(f"/api/v1/targets/{target['id']}/check")
    second = await service_client.post(f"/api/v1/targets/{target['id']}/check")
    assert first.status == 201
    assert second.status == 201

    history_response = await service_client.get(f"/api/v1/targets/{target['id']}/checks")
    assert history_response.status == 200
    history = history_response.json()
    assert len(history) == 2
    assert history[0]["id"] == second.json()["id"]
    assert history[1]["id"] == first.json()["id"]

    status_response = await service_client.get(f"/api/v1/targets/{target['id']}/status")
    assert status_response.status == 200
    assert status_response.json()["id"] == second.json()["id"]


async def test_manual_tcp_check_up(service_client):
    server = await asyncio.start_server(lambda reader, writer: writer.close(), "127.0.0.1", 0)
    try:
        port = server.sockets[0].getsockname()[1]
        target = await create_tcp_target(service_client, "127.0.0.1", port)

        response = await service_client.post(f"/api/v1/targets/{target['id']}/check")

        assert response.status == 201
        check = response.json()
        assert check["target_id"] == target["id"]
        assert check["status"] == "up"
        assert check["protocol"] == "tcp"
        assert check["latency_ms"] >= 0
        assert "error_message" not in check
    finally:
        server.close()
        await server.wait_closed()


async def test_manual_tcp_check_down(service_client):
    server = await asyncio.start_server(lambda reader, writer: writer.close(), "127.0.0.1", 0)
    port = server.sockets[0].getsockname()[1]
    server.close()
    await server.wait_closed()

    target = await create_tcp_target(service_client, "127.0.0.1", port)

    response = await service_client.post(f"/api/v1/targets/{target['id']}/check")

    assert response.status == 201
    check = response.json()
    assert check["status"] == "down"
    assert check["protocol"] == "tcp"
    assert check["latency_ms"] >= 0
    assert "error_message" in check


async def test_manual_check_target_not_found(service_client):
    response = await service_client.post("/api/v1/targets/999999/check")

    assert response.status == 404
    assert response.json()["error"] == "target not found"


async def test_target_status_without_checks(service_client, mockserver):
    target = await create_http_target(service_client, mockserver.url("/health"))

    response = await service_client.get(f"/api/v1/targets/{target['id']}/status")

    assert response.status == 404
    assert response.json()["error"] == "target has no checks"
