async def create_http_target(service_client, url, expected_status=200):
    response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "Alerted HTTP target",
            "type": "http",
            "url": url,
            "expected_status_code": expected_status,
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )
    assert response.status == 201
    return response.json()


async def test_manual_down_check_creates_active_alert(service_client, mockserver):
    @mockserver.handler("/health")
    async def health(request):
        return mockserver.make_response("broken", status=503)

    target = await create_http_target(service_client, mockserver.url("/health"))

    check_response = await service_client.post(f"/api/v1/targets/{target['id']}/check")
    assert check_response.status == 201
    assert check_response.json()["status"] == "down"

    active_response = await service_client.get("/api/v1/alerts/active")
    assert active_response.status == 200
    active_alerts = active_response.json()
    assert len(active_alerts) == 1
    assert active_alerts[0]["target_id"] == target["id"]
    assert active_alerts[0]["type"] == "target_down"
    assert active_alerts[0]["severity"] == "critical"
    assert "resolved_at" not in active_alerts[0]


async def test_repeated_down_checks_do_not_duplicate_active_alert(service_client, mockserver):
    @mockserver.handler("/health")
    async def health(request):
        return mockserver.make_response("broken", status=503)

    target = await create_http_target(service_client, mockserver.url("/health"))

    first = await service_client.post(f"/api/v1/targets/{target['id']}/check")
    second = await service_client.post(f"/api/v1/targets/{target['id']}/check")
    assert first.status == 201
    assert second.status == 201

    active_response = await service_client.get("/api/v1/alerts/active")
    assert active_response.status == 200
    assert len(active_response.json()) == 1


async def test_recovery_resolves_active_alert(service_client, mockserver):
    state = {"status": 503}

    @mockserver.handler("/health")
    async def health(request):
        return mockserver.make_response("response", status=state["status"])

    target = await create_http_target(service_client, mockserver.url("/health"))

    down_response = await service_client.post(f"/api/v1/targets/{target['id']}/check")
    assert down_response.status == 201
    assert down_response.json()["status"] == "down"

    state["status"] = 200
    up_response = await service_client.post(f"/api/v1/targets/{target['id']}/check")
    assert up_response.status == 201
    assert up_response.json()["status"] == "up"

    active_response = await service_client.get("/api/v1/alerts/active")
    assert active_response.status == 200
    assert active_response.json() == []

    all_response = await service_client.get("/api/v1/alerts")
    assert all_response.status == 200
    alerts = all_response.json()
    assert len(alerts) == 1
    assert alerts[0]["target_id"] == target["id"]
    assert alerts[0]["type"] == "target_down"
    assert "resolved_at" in alerts[0]
