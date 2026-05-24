async def test_create_http_target(service_client):
    response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "Main website",
            "type": "http",
            "url": "https://example.com/health",
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )

    assert response.status == 201
    data = response.json()
    assert data.pop("id") > 0
    assert data == {
        "name": "Main website",
        "type": "http",
        "url": "https://example.com/health",
        "method": "GET",
        "expected_status_code": 200,
        "interval_seconds": 30,
        "timeout_ms": 1000,
        "is_active": True,
    }


async def test_list_targets_empty(service_client):
    response = await service_client.get("/api/v1/targets")

    assert response.status == 200
    assert response.json() == []


async def test_list_targets(service_client):
    http_response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "Main website",
            "type": "http",
            "url": "https://example.com/health",
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )
    tcp_response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "Local Postgres",
            "type": "tcp",
            "host": "localhost",
            "port": 5432,
            "interval_seconds": 10,
            "timeout_ms": 500,
        },
    )

    assert http_response.status == 201
    assert tcp_response.status == 201

    response = await service_client.get("/api/v1/targets")

    assert response.status == 200
    targets = response.json()
    assert len(targets) == 2
    assert targets[0]["id"] == http_response.json()["id"]
    assert targets[1]["id"] == tcp_response.json()["id"]
    assert targets[0] == {
        "id": http_response.json()["id"],
        "name": "Main website",
        "type": "http",
        "url": "https://example.com/health",
        "method": "GET",
        "expected_status_code": 200,
        "interval_seconds": 30,
        "timeout_ms": 1000,
        "is_active": True,
    }
    assert targets[1] == {
        "id": tcp_response.json()["id"],
        "name": "Local Postgres",
        "type": "tcp",
        "host": "localhost",
        "port": 5432,
        "interval_seconds": 10,
        "timeout_ms": 500,
        "is_active": True,
    }


async def test_create_tcp_target(service_client):
    response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "Local Postgres",
            "type": "tcp",
            "host": "localhost",
            "port": 5432,
            "interval_seconds": 10,
            "timeout_ms": 500,
        },
    )

    assert response.status == 201
    data = response.json()
    assert data.pop("id") > 0
    assert data == {
        "name": "Local Postgres",
        "type": "tcp",
        "host": "localhost",
        "port": 5432,
        "interval_seconds": 10,
        "timeout_ms": 500,
        "is_active": True,
    }


async def test_reject_invalid_http_target(service_client):
    response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "Broken website",
            "type": "http",
            "interval_seconds": 1,
            "timeout_ms": 1000,
        },
    )

    assert response.status == 400
    assert response.json()["error"] == "interval_seconds must be at least 5"


async def test_reject_invalid_tcp_port(service_client):
    response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "Broken TCP",
            "type": "tcp",
            "host": "localhost",
            "port": 70000,
            "interval_seconds": 10,
            "timeout_ms": 500,
        },
    )

    assert response.status == 400
    assert response.json()["error"] == "tcp target port must be between 1 and 65535"
