#include <web/handlers/openapi_handler.hpp>

#include <userver/http/content_type.hpp>
#include <userver/server/http/http_request.hpp>

namespace netwatch::api_gateway::web {
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
    { "name": "auth" },
    { "name": "targets" },
    { "name": "checks" },
    { "name": "alerts" },
    { "name": "notifications" }
  ],
  "paths": {
    "/ping": {
      "get": {
        "tags": ["health"],
        "summary": "Healthcheck",
        "security": [],
        "responses": {
          "200": { "description": "Service is alive" }
        }
      }
    },
    "/api/v1/auth/register": {
      "post": {
        "tags": ["auth"],
        "summary": "Register user",
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": { "$ref": "#/components/schemas/AuthCredentials" },
              "example": {
                "email": "user@example.com",
                "password": "password123"
              }
            }
          }
        },
        "responses": {
          "201": {
            "description": "User registered and access token issued",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/AuthResponse" }
              }
            }
          },
          "400": {
            "description": "Validation error or duplicate email",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      }
    },
    "/api/v1/auth/login": {
      "post": {
        "tags": ["auth"],
        "summary": "Login user",
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": { "$ref": "#/components/schemas/AuthCredentials" },
              "example": {
                "email": "user@example.com",
                "password": "password123"
              }
            }
          }
        },
        "responses": {
          "200": {
            "description": "Access token issued",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/AuthResponse" }
              }
            }
          },
          "401": {
            "description": "Invalid credentials",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      }
    },
    "/api/v1/auth/me": {
      "get": {
        "tags": ["auth"],
        "summary": "Get current user by bearer token",
        "security": [
          { "BearerAuth": [] }
        ],
        "responses": {
          "200": {
            "description": "Current user session",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/ValidatedSession" }
              }
            }
          },
          "401": {
            "description": "Missing or invalid bearer token",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      }
    },
    "/api/v1/auth/verify-email": {
      "post": {
        "tags": ["auth"],
        "summary": "Verify user email by token",
        "security": [],
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": { "$ref": "#/components/schemas/VerifyEmailRequest" },
              "example": {
                "token": "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
              }
            }
          }
        },
        "responses": {
          "200": {
            "description": "Email verified",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/ValidatedSession" }
              }
            }
          },
          "400": {
            "description": "Invalid or expired verification token",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      }
    },
    "/api/v1/auth/resend-verification-email": {
      "post": {
        "tags": ["auth"],
        "summary": "Send another verification email to current user",
        "security": [
          { "BearerAuth": [] }
        ],
        "responses": {
          "200": {
            "description": "Verification email sent or account already verified",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/ValidatedSession" }
              }
            }
          },
          "401": {
            "description": "Missing or invalid bearer token",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      }
    },
    "/api/v1/targets": {
      "get": {
        "tags": ["targets"],
        "summary": "List active targets",
        "security": [
          { "BearerAuth": [] }
        ],
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
        "security": [
          { "BearerAuth": [] }
        ],
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
        "security": [
          { "BearerAuth": [] }
        ],
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
        "security": [
          { "BearerAuth": [] }
        ],
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
        "security": [
          { "BearerAuth": [] }
        ],
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
        "security": [
          { "BearerAuth": [] }
        ],
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
        "security": [
          { "BearerAuth": [] }
        ],
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
        "security": [
          { "BearerAuth": [] }
        ],
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
        "security": [
          { "BearerAuth": [] }
        ],
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
        "security": [
          { "BearerAuth": [] }
        ],
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
    },
    "/api/v1/notifications/recipients": {
      "get": {
        "tags": ["notifications"],
        "summary": "List email notification recipients",
        "security": [
          { "BearerAuth": [] }
        ],
        "responses": {
          "200": {
            "description": "Email notification recipients",
            "content": {
              "application/json": {
                "schema": {
                  "type": "array",
                  "items": { "$ref": "#/components/schemas/EmailRecipient" }
                }
              }
            }
          }
        }
      },
      "post": {
        "tags": ["notifications"],
        "summary": "Enable email notifications for current account email",
        "security": [
          { "BearerAuth": [] }
        ],
        "responses": {
          "201": {
            "description": "Created or re-enabled recipient for the authenticated user's email",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/EmailRecipient" }
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
    "/api/v1/notifications/recipients/{id}": {
      "parameters": [
        { "$ref": "#/components/parameters/RecipientId" }
      ],
      "get": {
        "tags": ["notifications"],
        "summary": "Get email notification recipient by id",
        "security": [
          { "BearerAuth": [] }
        ],
        "responses": {
          "200": {
            "description": "Email notification recipient",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/EmailRecipient" }
              }
            }
          },
          "404": {
            "description": "Email recipient not found",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      },
      "patch": {
        "tags": ["notifications"],
        "summary": "Update email notification recipient",
        "security": [
          { "BearerAuth": [] }
        ],
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": { "$ref": "#/components/schemas/UpdateEmailRecipientRequest" },
              "example": {
                "is_enabled": false
              }
            }
          }
        },
        "responses": {
          "200": {
            "description": "Updated email notification recipient",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/EmailRecipient" }
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
            "description": "Email recipient not found",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      },
      "delete": {
        "tags": ["notifications"],
        "summary": "Disable email notification recipient",
        "security": [
          { "BearerAuth": [] }
        ],
        "responses": {
          "204": { "description": "Email recipient disabled" },
          "404": {
            "description": "Email recipient not found",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      }
    },
    "/api/v1/notifications/deliveries": {
      "get": {
        "tags": ["notifications"],
        "summary": "List notification delivery attempts",
        "security": [
          { "BearerAuth": [] }
        ],
        "parameters": [
          {
            "name": "limit",
            "in": "query",
            "required": false,
            "schema": {
              "type": "integer",
              "format": "int32",
              "minimum": 1,
              "maximum": 500,
              "default": 100
            }
          },
          {
            "name": "status",
            "in": "query",
            "required": false,
            "schema": {
              "type": "string",
              "enum": ["pending", "sending", "retry_scheduled", "sent", "skipped", "failed"]
            }
          },
          {
            "name": "event_type",
            "in": "query",
            "required": false,
            "schema": {
              "type": "string",
              "enum": ["alert.opened", "alert.resolved"]
            }
          },
          {
            "name": "recipient_email",
            "in": "query",
            "required": false,
            "schema": {
              "type": "string",
              "format": "email"
            }
          }
        ],
        "responses": {
          "200": {
            "description": "Notification deliveries",
            "content": {
              "application/json": {
                "schema": {
                  "type": "array",
                  "items": { "$ref": "#/components/schemas/NotificationDelivery" }
                }
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
    "/api/v1/notifications/deliveries/{id}/retry": {
      "parameters": [
        { "$ref": "#/components/parameters/DeliveryId" }
      ],
      "post": {
        "tags": ["notifications"],
        "summary": "Retry failed or scheduled notification delivery",
        "security": [
          { "BearerAuth": [] }
        ],
        "responses": {
          "200": {
            "description": "Delivery moved back to pending",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/NotificationDelivery" }
              }
            }
          },
          "404": {
            "description": "Delivery not found or cannot be retried",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/Error" }
              }
            }
          }
        }
      }
    },
    "/api/v1/notifications/test-email": {
      "post": {
        "tags": ["notifications"],
        "summary": "Queue a test email notification",
        "security": [
          { "BearerAuth": [] }
        ],
        "requestBody": {
          "required": false,
          "content": {
            "application/json": {
              "schema": { "$ref": "#/components/schemas/SendTestEmailRequest" },
              "example": {
                "email": "alerts@example.com"
              }
            }
          }
        },
        "responses": {
          "202": {
            "description": "Test email queued",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/SendTestEmailResponse" }
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
    "/api/v1/targets/{id}/notifications": {
      "parameters": [
        { "$ref": "#/components/parameters/TargetId" }
      ],
      "get": {
        "tags": ["notifications"],
        "summary": "Get target notification settings",
        "security": [
          { "BearerAuth": [] }
        ],
        "responses": {
          "200": {
            "description": "Target notification settings",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/TargetNotificationSettings" }
              }
            }
          },
          "401": {
            "description": "Bearer token is missing or invalid",
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
      "patch": {
        "tags": ["notifications"],
        "summary": "Update target notification settings",
        "security": [
          { "BearerAuth": [] }
        ],
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": { "$ref": "#/components/schemas/UpdateTargetNotificationSettingsRequest" },
              "example": {
                "email_enabled": false
              }
            }
          }
        },
        "responses": {
          "200": {
            "description": "Updated target notification settings",
            "content": {
              "application/json": {
                "schema": { "$ref": "#/components/schemas/TargetNotificationSettings" }
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
          "401": {
            "description": "Bearer token is missing or invalid",
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
      }
    }
  },
  "components": {
    "securitySchemes": {
      "BearerAuth": {
        "type": "http",
        "scheme": "bearer"
      }
    },
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
      },
      "RecipientId": {
        "name": "id",
        "in": "path",
        "required": true,
        "schema": {
          "type": "integer",
          "format": "int64",
          "minimum": 1
        }
      },
      "DeliveryId": {
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
      "AuthCredentials": {
        "type": "object",
        "required": ["email", "password"],
        "properties": {
          "email": { "type": "string", "format": "email" },
          "password": { "type": "string", "minLength": 8 }
        }
      },
      "AuthUser": {
        "type": "object",
        "required": ["id", "email", "created_at", "updated_at", "email_verified"],
        "properties": {
          "id": { "type": "integer", "format": "int64" },
          "email": { "type": "string", "format": "email" },
          "created_at": { "type": "string", "format": "date-time" },
          "updated_at": { "type": "string", "format": "date-time" },
          "email_verified": { "type": "boolean" },
          "email_verified_at": { "type": "string", "format": "date-time" }
        }
      },
      "VerifyEmailRequest": {
        "type": "object",
        "required": ["token"],
        "properties": {
          "token": { "type": "string", "minLength": 1 }
        }
      },
      "AuthResponse": {
        "type": "object",
        "required": ["user", "access_token", "expires_at"],
        "properties": {
          "user": { "$ref": "#/components/schemas/AuthUser" },
          "access_token": { "type": "string" },
          "expires_at": { "type": "string", "format": "date-time" }
        }
      },
      "ValidatedSession": {
        "type": "object",
        "required": ["user", "expires_at"],
        "properties": {
          "user": { "$ref": "#/components/schemas/AuthUser" },
          "expires_at": { "type": "string", "format": "date-time" }
        }
      },
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
      "UpdateEmailRecipientRequest": {
        "type": "object",
        "properties": {
          "is_enabled": { "type": "boolean" }
        }
      },
      "EmailRecipient": {
        "type": "object",
        "required": ["id", "email", "is_enabled", "created_at", "updated_at"],
        "properties": {
          "id": { "type": "integer", "format": "int64" },
          "user_id": { "type": "integer", "format": "int64" },
          "email": { "type": "string", "format": "email" },
          "is_enabled": { "type": "boolean" },
          "created_at": { "type": "string", "format": "date-time" },
          "updated_at": { "type": "string", "format": "date-time" }
        }
      },
      "NotificationDelivery": {
        "type": "object",
        "required": [
          "id",
          "event_id",
          "event_type",
          "recipient_email",
          "channel",
          "status",
          "attempts",
          "error_message",
          "next_retry_at",
          "created_at",
          "updated_at",
          "delivered_at"
        ],
        "properties": {
          "id": { "type": "integer", "format": "int64" },
          "user_id": { "type": "integer", "format": "int64" },
          "target_id": { "type": "integer", "format": "int64" },
          "event_id": { "type": "string" },
          "event_type": { "type": "string", "enum": ["alert.opened", "alert.resolved"] },
          "recipient_email": { "type": "string" },
          "channel": { "type": "string", "enum": ["email"] },
          "status": { "type": "string", "enum": ["pending", "sending", "retry_scheduled", "sent", "skipped", "failed"] },
          "attempts": { "type": "integer", "format": "int32" },
          "error_message": { "type": "string" },
          "next_retry_at": { "type": "string", "format": "date-time" },
          "created_at": { "type": "string", "format": "date-time" },
          "updated_at": { "type": "string", "format": "date-time" },
          "delivered_at": { "type": "string", "format": "date-time" }
        }
      },
      "SendTestEmailRequest": {
        "type": "object",
        "properties": {
          "email": {
            "type": "string",
            "format": "email",
            "description": "Optional direct recipient. When omitted, the test email is queued for all enabled recipients."
          }
        }
      },
      "SendTestEmailResponse": {
        "type": "object",
        "required": ["event_id", "recipients_count", "deliveries_count"],
        "properties": {
          "event_id": { "type": "string" },
          "recipients_count": { "type": "integer", "format": "int64" },
          "deliveries_count": { "type": "integer", "format": "int64" }
        }
      },
      "UpdateTargetNotificationSettingsRequest": {
        "type": "object",
        "required": ["email_enabled"],
        "properties": {
          "email_enabled": { "type": "boolean" }
        }
      },
      "TargetNotificationSettings": {
        "type": "object",
        "required": ["user_id", "target_id", "email_enabled", "created_at", "updated_at"],
        "properties": {
          "user_id": { "type": "integer", "format": "int64" },
          "target_id": { "type": "integer", "format": "int64" },
          "email_enabled": { "type": "boolean" },
          "created_at": { "type": "string", "format": "date-time" },
          "updated_at": { "type": "string", "format": "date-time" }
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

}  // namespace netwatch::api_gateway::web
