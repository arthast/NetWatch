import asyncio


async def test_scheduler_runs_due_target_check(service_client, mockserver):
    @mockserver.handler("/health")
    async def health(request):
        return mockserver.make_response("OK", status=200)

    create_response = await service_client.post(
        "/api/v1/targets",
        json={
            "name": "Scheduled HTTP target",
            "type": "http",
            "url": mockserver.url("/health"),
            "expected_status_code": 200,
            "interval_seconds": 5,
            "timeout_ms": 1000,
        },
    )
    assert create_response.status == 201
    target = create_response.json()

    await service_client.run_periodic("target-check-scheduler")

    checks = []
    for _ in range(20):
        checks_response = await service_client.get(f"/api/v1/targets/{target['id']}/checks")
        assert checks_response.status == 200
        checks = checks_response.json()
        if checks:
            break
        await asyncio.sleep(0.05)

    assert len(checks) == 1
    assert checks[0]["target_id"] == target["id"]
    assert checks[0]["status"] == "up"
