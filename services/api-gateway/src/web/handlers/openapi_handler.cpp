#include <web/handlers/openapi_handler.hpp>

#include <userver/http/content_type.hpp>
#include <userver/server/http/http_request.hpp>

namespace monitor_service::web {
namespace {

constexpr std::string_view kOpenApiSpec = R"json({
  "openapi": "3.0.3",
  "info": {
    "title": "NetWatch API",
    "version": "0.3.0",
    "description": "HTTP API for managing network targets, running checks, reading check history, and viewing alerts."
  },
  "servers": [
    {
      "url": "/",
      "description": "Current NetWatch service"
    }
  ],
  "tags": [
    { "name": "health" },
    { "name": "targets" },
    { "name": "checks" },
    { "name": "alerts" }
  ],
  "paths": {
    "/ping": {
      "get": {
        "tags": ["health"],
        "summary": "Healthcheck",
        "responses": {
          "200": { "description": "Service is alive" }
        }
      }
    },
    "/api/v1/targets": {
      "get": {
        "tags": ["targets"],
        "summary": "List active targets",
        "responses": {
          "200": {
            "description": "Active targets",
            "content": {
              "application/json": {
                "schema": {
                  "type": "array",
                  "items": { "$ref": "#/components/schemas/Target" }
                }
              }
            }
          }
        }
      },
      "post": {
        "tags": ["targets"],
        "summary": "Create target",
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": { "$ref": "#/components/schemas/CreateTargetRequest" },
              "examples": {
                "http": {
                  "value": {
                    "name": "Main website",
                    "type": "http",
                    "url": "https://example.com/health",
                    "method": "GET",
                    "expected_status_code": 200,
                    "interval_seconds": 30,
                    "timeout_ms": 1000
                  }
                },
                "tcp": {
                  "value": {
                    "name": "Local Postgres",
                    "type": "tcp",
                    "host": "localhost",
                    "port": 5432,
                    "interval_seconds": 10,
                    "timeout_ms": 500
                  }
                }
              }
            }
          }
        },
        "responses": {
          "201": {
            "description": "Created target",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Target" }
              }
            }
          },
          "400": {
            "description": "Validation error",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      }
    },
    "/api/v1/targets/{id}": {
      "parameters": [
        { "$ref": "#/components/parameters/TargetId" }
      ],
      "get": {
        "tags": ["targets"],
        "summary": "Get target by id",
        "responses": {
          "200": {
            "description": "Target",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Target" }
              }
            }
          },
          "404": {
            "description": "Target not found",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      },
      "patch": {
        "tags": ["targets"],
        "summary": "Update target",
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": { "$ref": "#/components/schemas/UpdateTargetRequest" }
            }
          }
        },
        "responses": {
          "200": {
            "description": "Updated target",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Target" }
              }
            }
          },
          "400": {
            "description": "Validation error",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          },
          "404": {
            "description": "Target not found",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      },
      "delete": {
        "tags": ["targets"],
        "summary": "Soft-delete target",
        "responses": {
          "204": { "description": "Target deactivated" },
          "404": {
            "description": "Target not found",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      }
    },
    "/api/v1/targets/{id}/check": {
      "parameters": [
        { "$ref": "#/components/parameters/TargetId" }
      ],
      "post": {
        "tags": ["checks"],
        "summary": "Run manual check",
        "responses": {
          "201": {
            "description": "Saved check result",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/CheckResult" }
              }
            }
          },
          "404": {
            "description": "Target not found",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      }
    },
    "/api/v1/targets/{id}/checks": {
      "parameters": [
        { "$ref": "#/components/parameters/TargetId" }
      ],
      "get": {
        "tags": ["checks"],
        "summary": "List target check results",
        "responses": {
          "200": {
            "description": "Check history",
            "content": {
              "application/json": {
                "schema": {
                  "type": "array",
                  "items": { "$ref": "#/components/schemas/CheckResult" }
                }
              }
            }
          }
        }
      }
    },
    "/api/v1/targets/{id}/status": {
      "parameters": [
        { "$ref": "#/components/parameters/TargetId" }
      ],
      "get": {
        "tags": ["checks"],
        "summary": "Get latest target status",
        "responses": {
          "200": {
            "description": "Latest check result",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/CheckResult" }
              }
            }
          },
          "404": {
            "description": "Target or status not found",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      }
    },
    "/api/v1/alerts": {
      "get": {
        "tags": ["alerts"],
        "summary": "List all alerts",
        "responses": {
          "200": {
            "description": "Alerts",
            "content": {
              "application/json": {
                "schema": {
                  "type": "array",
                  "items": { "$ref": "#/components/schemas/Alert" }
                }
              }
            }
          }
        }
      }
    },
    "/api/v1/alerts/active": {
      "get": {
        "tags": ["alerts"],
        "summary": "List active alerts",
        "responses": {
          "200": {
            "description": "Active alerts",
            "content": {
              "application/json": {
                "schema": {
                  "type": "array",
                  "items": { "$ref": "#/components/schemas/Alert" }
                }
              }
            }
          }
        }
      }
    }
  },
  "components": {
    "parameters": {
      "TargetId": {
        "name": "id",
        "in": "path",
        "required": true,
        "schema": {
          "type": "integer",
          "format": "int64",
          "minimum": 1
        }
      }
    },
    "schemas": {
      "CreateTargetRequest": {
        "oneOf": [
          { "$ref": "#/components/schemas/CreateHttpTargetRequest" },
          { "$ref": "#/components/schemas/CreateTcpTargetRequest" }
        ]
      },
      "CreateHttpTargetRequest": {
        "type": "object",
        "required": ["name", "type", "url", "interval_seconds", "timeout_ms"],
        "properties": {
          "name": { "type": "string" },
          "type": { "type": "string", "enum": ["http"] },
          "url": { "type": "string", "format": "uri" },
          "method": { "type": "string", "default": "GET" },
          "expected_status_code": { "type": "integer", "default": 200 },
          "interval_seconds": { "type": "integer", "minimum": 5 },
          "timeout_ms": { "type": "integer", "minimum": 1 }
        }
      },
      "CreateTcpTargetRequest": {
        "type": "object",
        "required": ["name", "type", "host", "port", "interval_seconds", "timeout_ms"],
        "properties": {
          "name": { "type": "string" },
          "type": { "type": "string", "enum": ["tcp"] },
          "host": { "type": "string" },
          "port": { "type": "integer", "minimum": 1, "maximum": 65535 },
          "interval_seconds": { "type": "integer", "minimum": 5 },
          "timeout_ms": { "type": "integer", "minimum": 1 }
        }
      },
      "UpdateTargetRequest": {
        "type": "object",
        "properties": {
          "name": { "type": "string" },
          "type": { "type": "string", "enum": ["http", "tcp"] },
          "url": { "type": "string" },
          "method": { "type": "string" },
          "expected_status_code": { "type": "integer" },
          "host": { "type": "string" },
          "port": { "type": "integer", "minimum": 1, "maximum": 65535 },
          "interval_seconds": { "type": "integer", "minimum": 5 },
          "timeout_ms": { "type": "integer", "minimum": 1 }
        }
      },
      "Target": {
        "type": "object",
        "required": ["id", "name", "type", "interval_seconds", "timeout_ms", "is_active"],
        "properties": {
          "id": { "type": "integer", "format": "int64" },
          "name": { "type": "string" },
          "type": { "type": "string", "enum": ["http", "tcp"] },
          "url": { "type": "string" },
          "method": { "type": "string" },
          "expected_status_code": { "type": "integer" },
          "host": { "type": "string" },
          "port": { "type": "integer" },
          "interval_seconds": { "type": "integer" },
          "timeout_ms": { "type": "integer" },
          "is_active": { "type": "boolean" }
        }
      },
      "CheckResult": {
        "type": "object",
        "required": ["id", "target_id", "status", "protocol", "checked_at"],
        "properties": {
          "id": { "type": "integer", "format": "int64" },
          "target_id": { "type": "integer", "format": "int64" },
          "status": { "type": "string", "enum": ["up", "down"] },
          "protocol": { "type": "string", "enum": ["http", "tcp"] },
          "http_status": { "type": "integer" },
          "latency_ms": { "type": "integer" },
          "error_message": { "type": "string" },
          "checked_at": { "type": "string", "format": "date-time" }
        }
      },
      "Alert": {
        "type": "object",
        "required": ["id", "target_id", "type", "severity", "message", "created_at"],
        "properties": {
          "id": { "type": "integer", "format": "int64" },
          "target_id": { "type": "integer", "format": "int64" },
          "type": { "type": "string", "enum": ["target_down", "target_recovered", "high_latency"] },
          "severity": { "type": "string", "enum": ["warning", "critical"] },
          "message": { "type": "string" },
          "created_at": { "type": "string", "format": "date-time" },
          "resolved_at": { "type": "string", "format": "date-time" }
        }
      },
      "Error": {
        "type": "object",
        "required": ["error"],
        "properties": {
          "error": { "type": "string" }
        }
      }
    }
  }
})json";

}  // namespace

OpenApiHandler::OpenApiHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context) {}

std::string OpenApiHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  request.GetHttpResponse().SetContentType(
      userver::http::content_type::kApplicationJson);
  return std::string{kOpenApiSpec};
}

}  // namespace monitor_service::web
