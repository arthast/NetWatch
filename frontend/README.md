# NetWatch frontend

Static browser console for the NetWatch API.

## Docker Compose

The normal runtime path is the `frontend` service in Docker Compose:

```bash
docker compose -f docker-compose.images.yml up -d --build frontend
```

Open `http://localhost:3000`.

The frontend sends API calls to relative paths such as `/api/v1/targets`.
Nginx proxies those requests to `api-gateway:8080` inside the Compose network,
so CORS is not needed.

## Local dev helper

Run it with the bundled proxy server:

```bash
python3 frontend/dev_server.py --api http://localhost:8081
```

For the current remote test stack:

```bash
python3 frontend/dev_server.py --api http://81.26.189.42:8081
```

Open `http://127.0.0.1:5173`.
