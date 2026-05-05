# Start via `make test-debug` or `make test-release`


async def test_basic(service_client):
    response = await service_client.get("/ping")
    assert response.status == 200
