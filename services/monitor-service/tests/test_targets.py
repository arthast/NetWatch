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


async def test_get_target_by_id(service_client):
    create_response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "Main website",
            "type": "http",
            "url": "https://example.com/health",
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )
    assert create_response.status == 201
    created = create_response.json()

    response = await service_client.get(f"/api/v1/targets/{created['id']}")

    assert response.status == 200
    assert response.json() == created


async def test_get_target_by_id_not_found(service_client):
    response = await service_client.get("/api/v1/targets/999999")

    assert response.status == 404
    assert response.json()["error"] == "target not found"


async def test_get_target_by_id_rejects_invalid_id(service_client):
    response = await service_client.get("/api/v1/targets/not-a-number")

    assert response.status == 400
    assert response.json()["error"] == "target id must be a positive integer"


async def test_patch_target(service_client):
    create_response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "Main website",
            "type": "http",
            "url": "https://example.com/health",
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )
    assert create_response.status == 201
    target_id = create_response.json()["id"]

    response = await service_client.patch(
        f"/api/v1/targets/{target_id}",
        json={
            "name": "Main website v2",
            "method": "HEAD",
            "expected_status_code": 204,
            "interval_seconds": 60,
            "timeout_ms": 1500,
        },
    )

    assert response.status == 200
    assert response.json() == {
        "id": target_id,
        "name": "Main website v2",
        "type": "http",
        "url": "https://example.com/health",
        "method": "HEAD",
        "expected_status_code": 204,
        "interval_seconds": 60,
        "timeout_ms": 1500,
        "is_active": True,
    }

    get_response = await service_client.get(f"/api/v1/targets/{target_id}")
    assert get_response.status == 200
    assert get_response.json() == response.json()


async def test_patch_target_can_change_protocol(service_client):
    create_response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "Main website",
            "type": "http",
            "url": "https://example.com/health",
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )
    assert create_response.status == 201
    target_id = create_response.json()["id"]

    response = await service_client.patch(
        f"/api/v1/targets/{target_id}",
        json={
            "type": "tcp",
            "host": "localhost",
            "port": 5432,
            "interval_seconds": 10,
            "timeout_ms": 500,
        },
    )

    assert response.status == 200
    assert response.json() == {
        "id": target_id,
        "name": "Main website",
        "type": "tcp",
        "host": "localhost",
        "port": 5432,
        "interval_seconds": 10,
        "timeout_ms": 500,
        "is_active": True,
    }


async def test_patch_target_not_found(service_client):
    response = await service_client.patch(
        "/api/v1/targets/999999",
        json={"name": "Missing"},
    )

    assert response.status == 404
    assert response.json()["error"] == "target not found"


async def test_patch_target_rejects_invalid_result(service_client):
    create_response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "Main website",
            "type": "http",
            "url": "https://example.com/health",
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )
    assert create_response.status == 201
    target_id = create_response.json()["id"]

    response = await service_client.patch(
        f"/api/v1/targets/{target_id}",
        json={"host": "localhost"},
    )

    assert response.status == 400
    assert response.json()["error"] == "http target must not contain host"


async def test_patch_target_rejects_empty_patch(service_client):
    create_response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "Main website",
            "type": "http",
            "url": "https://example.com/health",
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )
    assert create_response.status == 201
    target_id = create_response.json()["id"]

    response = await service_client.patch(f"/api/v1/targets/{target_id}", json={})

    assert response.status == 400
    assert response.json()["error"] == "patch body must contain at least one field"


async def test_delete_target_soft_deletes(service_client):
    create_response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "Main website",
            "type": "http",
            "url": "https://example.com/health",
            "interval_seconds": 30,
            "timeout_ms": 1000,
        },
    )
    assert create_response.status == 201
    target_id = create_response.json()["id"]

    delete_response = await service_client.delete(f"/api/v1/targets/{target_id}")

    assert delete_response.status == 204

    get_response = await service_client.get(f"/api/v1/targets/{target_id}")
    assert get_response.status == 404
    assert get_response.json()["error"] == "target not found"

    list_response = await service_client.get("/api/v1/targets")
    assert list_response.status == 200
    assert list_response.json() == []


async def test_delete_target_not_found(service_client):
    response = await service_client.delete("/api/v1/targets/999999")

    assert response.status == 404
    assert response.json()["error"] == "target not found"


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
