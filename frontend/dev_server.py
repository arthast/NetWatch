#!/usr/bin/env python3
import argparse
import http.server
import json
import mimetypes
import pathlib
import socketserver
import urllib.error
import urllib.request


ROOT = pathlib.Path(__file__).resolve().parent


class NetWatchFrontendHandler(http.server.BaseHTTPRequestHandler):
    api_base = "http://localhost:8081"
    head_only = False

    def do_HEAD(self):
        self.head_only = True
        if self.path.startswith("/api/") or self.path in {
            "/ping",
            "/docs",
            "/openapi.json",
        }:
            self.proxy()
            return
        self.serve_static()

    def do_GET(self):
        self.head_only = False
        if self.path.startswith("/api/") or self.path in {
            "/ping",
            "/docs",
            "/openapi.json",
        }:
            self.proxy()
            return
        self.serve_static()

    def do_POST(self):
        self.proxy()

    def do_PATCH(self):
        self.proxy()

    def do_DELETE(self):
        self.proxy()

    def serve_static(self):
        path = self.path.split("?", 1)[0]
        if path == "/":
            path = "/index.html"
        file_path = (ROOT / path.lstrip("/")).resolve()
        if not str(file_path).startswith(str(ROOT)) or not file_path.is_file():
            self.send_error(404)
            return
        content = file_path.read_bytes()
        content_type = mimetypes.guess_type(file_path.name)[0] or "application/octet-stream"
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(content)))
        self.end_headers()
        if not self.head_only:
            self.wfile.write(content)

    def proxy(self):
        body = None
        if "Content-Length" in self.headers:
            body = self.rfile.read(int(self.headers["Content-Length"]))

        headers = {
            key: value
            for key, value in self.headers.items()
            if key.lower() not in {"host", "content-length", "accept-encoding", "connection"}
        }
        request = urllib.request.Request(
            self.api_base.rstrip("/") + self.path,
            data=body,
            headers=headers,
            method=self.command,
        )

        try:
            with urllib.request.urlopen(request, timeout=20) as response:
                response_body = response.read()
                self.send_response(response.status)
                self.copy_headers(response.headers, len(response_body))
                self.end_headers()
                if not self.head_only:
                    self.wfile.write(response_body)
        except urllib.error.HTTPError as error:
            response_body = error.read()
            self.send_response(error.code)
            self.copy_headers(error.headers, len(response_body))
            self.end_headers()
            if not self.head_only:
                self.wfile.write(response_body)
        except Exception as error:  # noqa: BLE001
            response_body = json.dumps({"error": str(error)}).encode()
            self.send_response(502)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(response_body)))
            self.end_headers()
            if not self.head_only:
                self.wfile.write(response_body)

    def copy_headers(self, headers, content_length):
        blocked = {"transfer-encoding", "connection", "content-encoding", "content-length"}
        for key, value in headers.items():
            if key.lower() not in blocked:
                self.send_header(key, value)
        self.send_header("Content-Length", str(content_length))

    def log_message(self, fmt, *args):
        print(f"{self.address_string()} - {fmt % args}")


class ReusableThreadingTCPServer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True


def main():
    parser = argparse.ArgumentParser(description="Serve NetWatch frontend with an API proxy.")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=5173)
    parser.add_argument("--api", default="http://localhost:8081")
    args = parser.parse_args()

    NetWatchFrontendHandler.api_base = args.api
    with ReusableThreadingTCPServer((args.host, args.port), NetWatchFrontendHandler) as server:
        print(f"NetWatch frontend: http://{args.host}:{args.port}", flush=True)
        print(f"Proxying API to: {args.api}", flush=True)
        server.serve_forever()


if __name__ == "__main__":
    main()
