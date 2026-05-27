async def test_swagger_ui(service_client):
    response = await service_client.get("/docs")

    assert response.status == 200
    assert "text/html" in response.headers["content-type"]
    assert "SwaggerUIBundle" in response.text
    assert "/openapi.json" in response.text


async def test_openapi_spec(service_client):
    response = await service_client.get("/openapi.json")

    assert response.status == 200
    assert response.headers["content-type"].startswith("application/json")
    spec = response.json()
    assert spec["openapi"] == "3.0.3"
    assert spec["info"]["title"] == "NetWatch API"
    assert "/api/v1/targets" in spec["paths"]
    assert "/api/v1/alerts/active" in spec["paths"]
