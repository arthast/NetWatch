const DEFAULT_API_BASE = "";

const state = {
  token: localStorage.getItem("netwatch.token") || "",
  session: JSON.parse(localStorage.getItem("netwatch.session") || "null"),
  apiBase: window.NETWATCH_API_BASE || DEFAULT_API_BASE,
  authMode: "login",
  view: "overview",
  targets: [],
  statuses: new Map(),
  alerts: [],
  activeAlerts: [],
  recipients: [],
  deliveries: [],
  selectedTargetId: null,
  selectedTargetDetails: null,
  selectedTargetChecks: [],
  selectedTargetNotifications: null,
  loading: false,
  noticeTimer: null,
};

const views = [
  { id: "overview", label: "Overview", icon: "layout-dashboard" },
  { id: "targets", label: "Targets", icon: "radio-tower" },
  { id: "alerts", label: "Alerts", icon: "siren" },
  { id: "notifications", label: "Notifications", icon: "mail-check" },
];

const app = document.querySelector("#app");

function icon(name, label = "") {
  return `<i data-lucide="${name}" class="icon-svg" aria-hidden="true"></i>${label ? `<span>${label}</span>` : ""}`;
}

function renderIcons() {
  if (window.lucide) {
    window.lucide.createIcons();
  }
}

function notify(title, message, type = "info") {
  window.clearTimeout(state.noticeTimer);
  const node = document.querySelector(".notice");
  if (!node) return;
  node.className = `notice show ${type === "error" ? "error" : ""}`;
  node.innerHTML = `<div class="notice-title">${escapeHtml(title)}</div><div class="notice-message">${escapeHtml(message)}</div>`;
  state.noticeTimer = window.setTimeout(() => {
    node.className = "notice";
  }, 3600);
}

function sleep(ms) {
  return new Promise((resolve) => window.setTimeout(resolve, ms));
}

function escapeHtml(value) {
  return String(value ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

function formatDate(value) {
  if (!value) return "—";
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) return value;
  return new Intl.DateTimeFormat(undefined, {
    month: "short",
    day: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
  }).format(date);
}

function endpoint(target) {
  if (!target) return "—";
  return target.type === "http" ? target.url : `${target.host}:${target.port}`;
}

function statusFor(targetId) {
  return state.statuses.get(Number(targetId));
}

function activeAlertFor(targetId) {
  return state.activeAlerts.find((alert) => Number(alert.target_id) === Number(targetId));
}

async function api(path, options = {}) {
  const headers = new Headers(options.headers || {});
  if (options.body !== undefined && !headers.has("Content-Type")) {
    headers.set("Content-Type", "application/json");
  }
  if (state.token) {
    headers.set("Authorization", `Bearer ${state.token}`);
  }

  const response = await fetch(`${state.apiBase}${path}`, {
    ...options,
    headers,
    body: options.body !== undefined ? JSON.stringify(options.body) : undefined,
  });

  if (response.status === 204) return null;
  const text = await response.text();
  const payload = text ? JSON.parse(text) : null;
  if (!response.ok) {
    const message = payload?.error || `${response.status} ${response.statusText}`;
    throw new Error(message);
  }
  return payload;
}

function persistAuth(auth) {
  state.token = auth.access_token;
  state.session = { user: auth.user, expires_at: auth.expires_at };
  localStorage.setItem("netwatch.token", state.token);
  localStorage.setItem("netwatch.session", JSON.stringify(state.session));
}

function clearAuth() {
  state.token = "";
  state.session = null;
  localStorage.removeItem("netwatch.token");
  localStorage.removeItem("netwatch.session");
}

async function boot() {
  render();
  if (state.token) {
    try {
      const session = await api("/api/v1/auth/me");
      state.session = session;
      localStorage.setItem("netwatch.session", JSON.stringify(session));
      await refreshAll();
    } catch (error) {
      clearAuth();
      render();
      notify("Session expired", error.message, "error");
    }
  }
}

async function refreshAll() {
  state.loading = true;
  render();
  try {
    const [targets, activeAlerts, alerts, recipients, deliveries] = await Promise.all([
      api("/api/v1/targets"),
      api("/api/v1/alerts/active"),
      api("/api/v1/alerts"),
      api("/api/v1/notifications/recipients"),
      api("/api/v1/notifications/deliveries?limit=50"),
    ]);
    state.targets = targets || [];
    state.activeAlerts = activeAlerts || [];
    state.alerts = alerts || [];
    state.recipients = recipients || [];
    state.deliveries = deliveries || [];
    await loadStatuses();
  } catch (error) {
    notify("Request failed", error.message, "error");
  } finally {
    state.loading = false;
    render();
  }
}

async function loadStatuses() {
  const pairs = await Promise.all(
    state.targets.map(async (target) => {
      try {
        return [target.id, await api(`/api/v1/targets/${target.id}/status`)];
      } catch (_error) {
        return [target.id, null];
      }
    }),
  );
  state.statuses = new Map(pairs);
}

function render() {
  if (!state.token || !state.session) {
    app.innerHTML = authMarkup();
  } else {
    app.innerHTML = shellMarkup();
  }
  bindEvents();
  renderIcons();
}

function authMarkup() {
  const isLogin = state.authMode === "login";
  return `
    <main class="auth-layout">
      <section class="auth-panel">
        <div class="auth-head">
          <div class="brand-mark">${icon("activity")}</div>
          <div>
            <h1 class="auth-title">NetWatch</h1>
            <div class="page-subtitle">Monitoring console</div>
          </div>
        </div>
        <div class="auth-tabs">
          <button class="tab-button ${isLogin ? "active" : ""}" data-auth-mode="login">Sign in</button>
          <button class="tab-button ${!isLogin ? "active" : ""}" data-auth-mode="register">Register</button>
        </div>
        <form class="form" data-action="auth">
          <div class="field">
            <label for="auth-email">Email</label>
            <input id="auth-email" name="email" type="email" autocomplete="email" required />
          </div>
          <div class="field">
            <label for="auth-password">Password</label>
            <input id="auth-password" name="password" type="password" autocomplete="${isLogin ? "current-password" : "new-password"}" minlength="8" required />
          </div>
          <button class="button primary" type="submit">${icon(isLogin ? "log-in" : "user-plus", isLogin ? "Sign in" : "Create account")}</button>
        </form>
      </section>
      <div class="notice"></div>
    </main>
  `;
}

function shellMarkup() {
  const current = views.find((view) => view.id === state.view) || views[0];
  return `
    <div class="app-shell ${state.loading ? "loading" : ""}">
      <aside class="sidebar">
        <div class="brand">
          <div class="brand-mark">${icon("activity")}</div>
          <div class="brand-copy">
            <div class="brand-title">NetWatch</div>
            <div class="brand-subtitle">Console</div>
          </div>
        </div>
        <nav class="nav">
          ${views
            .map(
              (view) => `
                <button class="nav-button ${state.view === view.id ? "active" : ""}" data-view="${view.id}" title="${view.label}">
                  ${icon(view.icon)}<span class="nav-label">${view.label}</span>
                </button>
              `,
            )
            .join("")}
        </nav>
        <div class="sidebar-footer">
          <div class="user-email">${escapeHtml(state.session.user.email)}</div>
          <button class="button" data-action="logout">${icon("log-out", "Sign out")}</button>
        </div>
      </aside>
      <section class="main">
        <header class="topbar">
          <div>
            <h1 class="page-title">${current.label}</h1>
            <p class="page-subtitle">${subtitleForView(state.view)}</p>
          </div>
          <div class="toolbar">
            <button class="button icon" data-action="refresh" title="Refresh">${icon("refresh-cw")}<span class="sr-only">Refresh</span></button>
            <button class="button primary" data-view="targets" title="Targets">${icon("plus", "Target")}</button>
          </div>
        </header>
        <main class="content">${contentMarkup()}</main>
      </section>
      ${drawerMarkup()}
      <div class="notice"></div>
    </div>
  `;
}

function subtitleForView(view) {
  const labels = {
    overview: "Fleet health, active incidents, and recent delivery status.",
    targets: "Create targets, run checks, and tune per-target notifications.",
    alerts: "Active and resolved alert lifecycle events.",
    notifications: "Email recipients and delivery attempts.",
  };
  return labels[view] || "";
}

function contentMarkup() {
  if (state.view === "targets") return targetsMarkup();
  if (state.view === "alerts") return alertsMarkup();
  if (state.view === "notifications") return notificationsMarkup();
  return overviewMarkup();
}

function overviewMarkup() {
  const downCount = state.targets.filter((target) => statusFor(target.id)?.status === "down").length;
  const upCount = state.targets.filter((target) => statusFor(target.id)?.status === "up").length;
  const pendingDeliveries = state.deliveries.filter((delivery) =>
    ["pending", "sending", "retry_scheduled"].includes(delivery.status),
  ).length;
  return `
    <div class="grid three">
      ${metricMarkup("Targets", state.targets.length, "radio-tower")}
      ${metricMarkup("Up", upCount, "circle-check")}
      ${metricMarkup("Active alerts", state.activeAlerts.length, "siren")}
    </div>
    <div class="grid two">
      <section class="surface">
        <div class="surface-header">
          <h2 class="surface-title">Target status</h2>
          <span class="pill ${downCount ? "down" : "ok"}">${downCount ? `${downCount} down` : "stable"}</span>
        </div>
        <div class="table-wrap">${targetsTableMarkup(state.targets.slice(0, 6))}</div>
      </section>
      <section class="surface">
        <div class="surface-header">
          <h2 class="surface-title">Notification queue</h2>
          <span class="pill ${pendingDeliveries ? "pending" : "sent"}">${pendingDeliveries ? `${pendingDeliveries} pending` : "clear"}</span>
        </div>
        <div class="surface-body">${deliveryListMarkup(state.deliveries.slice(0, 6))}</div>
      </section>
    </div>
  `;
}

function metricMarkup(label, value, iconName) {
  return `
    <section class="surface metric">
      <div>
        <div class="metric-value">${escapeHtml(value)}</div>
        <div class="metric-label">${escapeHtml(label)}</div>
      </div>
      <div class="metric-icon">${icon(iconName)}</div>
    </section>
  `;
}

function targetsMarkup() {
  return `
    <div class="grid two">
      <section class="surface">
        <div class="surface-header">
          <h2 class="surface-title">Targets</h2>
          <span class="pill">${state.targets.length}</span>
        </div>
        <div class="table-wrap">${targetsTableMarkup(state.targets)}</div>
      </section>
      <section class="surface">
        <div class="surface-header"><h2 class="surface-title">New target</h2></div>
        <div class="surface-body">${targetFormMarkup()}</div>
      </section>
    </div>
  `;
}

function targetFormMarkup() {
  const type = "http";
  return `
    <form class="form" data-action="create-target">
      <div class="segmented" data-target-type>
        <button class="segment active" type="button" data-type="http">HTTP</button>
        <button class="segment" type="button" data-type="tcp">TCP</button>
      </div>
      <input type="hidden" name="type" value="${type}" />
      <div class="field">
        <label for="target-name">Name</label>
        <input id="target-name" name="name" required value="Main website" />
      </div>
      <div class="field protocol http-fields">
        <label for="target-url">URL</label>
        <input id="target-url" name="url" type="url" value="http://81.26.189.42:8081/ping" />
      </div>
      <div class="field inline protocol http-fields">
        <div class="field">
          <label for="target-method">Method</label>
          <select id="target-method" name="method">
            <option>GET</option>
            <option>HEAD</option>
          </select>
        </div>
        <div class="field">
          <label for="target-status">Expected status</label>
          <input id="target-status" name="expected_status_code" type="number" value="200" min="100" max="599" />
        </div>
      </div>
      <div class="field protocol tcp-fields" hidden>
        <label for="target-host">Host</label>
        <input id="target-host" name="host" value="localhost" />
      </div>
      <div class="field protocol tcp-fields" hidden>
        <label for="target-port">Port</label>
        <input id="target-port" name="port" type="number" min="1" max="65535" value="8080" />
      </div>
      <div class="field inline">
        <div class="field">
          <label for="target-interval">Interval seconds</label>
          <input id="target-interval" name="interval_seconds" type="number" min="5" value="30" required />
        </div>
        <div class="field">
          <label for="target-timeout">Timeout ms</label>
          <input id="target-timeout" name="timeout_ms" type="number" min="1" value="1000" required />
        </div>
      </div>
      <button class="button primary" type="submit">${icon("plus", "Create target")}</button>
    </form>
  `;
}

function targetsTableMarkup(targets) {
  if (!targets.length) return `<div class="empty">No targets yet</div>`;
  return `
    <table>
      <thead>
        <tr>
          <th>Target</th>
          <th>Type</th>
          <th>Status</th>
          <th>Interval</th>
          <th></th>
        </tr>
      </thead>
      <tbody>
        ${targets.map(targetRowMarkup).join("")}
      </tbody>
    </table>
  `;
}

function targetRowMarkup(target) {
  const status = statusFor(target.id);
  const alert = activeAlertFor(target.id);
  const statusClass = alert ? "down" : status?.status === "up" ? "ok" : status?.status === "down" ? "down" : "";
  const statusText = alert ? "alert" : status?.status || "unknown";
  return `
    <tr>
      <td>
        <div class="target-name">
          <strong>${escapeHtml(target.name)}</strong>
          <span class="target-endpoint">${escapeHtml(endpoint(target))}</span>
        </div>
      </td>
      <td><span class="pill">${escapeHtml(target.type)}</span></td>
      <td><span class="pill ${statusClass}">${escapeHtml(statusText)}</span></td>
      <td>${escapeHtml(target.interval_seconds)}s</td>
      <td>
        <div class="row-actions">
          <button class="button icon" data-action="run-check" data-id="${target.id}" title="Run check">${icon("play")}<span class="sr-only">Run check</span></button>
          <button class="button icon" data-action="open-target" data-id="${target.id}" title="Details">${icon("panel-right-open")}<span class="sr-only">Details</span></button>
          <button class="button icon danger" data-action="delete-target" data-id="${target.id}" title="Delete">${icon("trash-2")}<span class="sr-only">Delete</span></button>
        </div>
      </td>
    </tr>
  `;
}

function alertsMarkup() {
  const rows = state.alerts.length ? state.alerts : state.activeAlerts;
  return `
    <section class="surface">
      <div class="surface-header">
        <h2 class="surface-title">Alerts</h2>
        <span class="pill ${state.activeAlerts.length ? "critical" : "ok"}">${state.activeAlerts.length} active</span>
      </div>
      <div class="table-wrap">
        ${
          rows.length
            ? `<table>
                <thead><tr><th>Alert</th><th>Target</th><th>Severity</th><th>Created</th><th>Resolved</th></tr></thead>
                <tbody>${rows
                  .map(
                    (alert) => `
                      <tr>
                        <td><strong>${escapeHtml(alert.type)}</strong><div class="target-endpoint">${escapeHtml(alert.message)}</div></td>
                        <td>#${escapeHtml(alert.target_id)}</td>
                        <td><span class="pill ${alert.severity}">${escapeHtml(alert.severity)}</span></td>
                        <td>${formatDate(alert.created_at)}</td>
                        <td>${formatDate(alert.resolved_at)}</td>
                      </tr>
                    `,
                  )
                  .join("")}</tbody>
              </table>`
            : `<div class="empty">No alerts</div>`
        }
      </div>
    </section>
  `;
}

function notificationsMarkup() {
  return `
    <div class="grid two">
      <section class="surface">
        <div class="surface-header">
          <h2 class="surface-title">Recipients</h2>
          <span class="pill">${state.recipients.length}</span>
        </div>
        <div class="surface-body">
          <form class="form" data-action="create-recipient">
            <div class="field">
              <label for="recipient-email">Email</label>
              <input id="recipient-email" name="email" type="email" required />
            </div>
            <button class="button primary" type="submit">${icon("mail-plus", "Add recipient")}</button>
          </form>
        </div>
        <div class="table-wrap">${recipientsTableMarkup()}</div>
      </section>
      <section class="surface">
        <div class="surface-header">
          <h2 class="surface-title">Deliveries</h2>
          <button class="button" data-action="test-email">${icon("send", "Test")}</button>
        </div>
        <div class="surface-body">${deliveryListMarkup(state.deliveries)}</div>
      </section>
    </div>
  `;
}

function recipientsTableMarkup() {
  if (!state.recipients.length) return `<div class="empty">No recipients</div>`;
  return `
    <table>
      <thead><tr><th>Email</th><th>Status</th><th></th></tr></thead>
      <tbody>
        ${state.recipients
          .map(
            (recipient) => `
              <tr>
                <td>${escapeHtml(recipient.email)}</td>
                <td><span class="pill ${recipient.is_enabled ? "ok" : ""}">${recipient.is_enabled ? "enabled" : "disabled"}</span></td>
                <td>
                  <div class="row-actions">
                    <button class="button icon" data-action="toggle-recipient" data-id="${recipient.id}" data-enabled="${recipient.is_enabled ? "false" : "true"}" title="Toggle">${icon(recipient.is_enabled ? "bell-off" : "bell")}<span class="sr-only">Toggle</span></button>
                    <button class="button icon danger" data-action="delete-recipient" data-id="${recipient.id}" title="Disable">${icon("trash-2")}<span class="sr-only">Disable</span></button>
                  </div>
                </td>
              </tr>
            `,
          )
          .join("")}
      </tbody>
    </table>
  `;
}

function deliveryListMarkup(deliveries) {
  if (!deliveries.length) return `<div class="empty">No deliveries</div>`;
  return `
    <div class="details">
      ${deliveries
        .map(
          (delivery) => `
            <div class="details-row">
              <div>
                <strong>${escapeHtml(delivery.recipient_email)}</strong>
                <div class="target-endpoint">${escapeHtml(delivery.event_type)} ${delivery.target_id ? `#${escapeHtml(delivery.target_id)}` : ""}</div>
              </div>
              <div class="details-value">
                <span class="pill ${escapeHtml(delivery.status)}">${escapeHtml(delivery.status)}</span>
                ${delivery.status === "failed" || delivery.status === "retry_scheduled" ? `<button class="button icon" data-action="retry-delivery" data-id="${delivery.id}" title="Retry">${icon("rotate-ccw")}<span class="sr-only">Retry</span></button>` : ""}
              </div>
            </div>
          `,
        )
        .join("")}
    </div>
  `;
}

function drawerMarkup() {
  const open = Boolean(state.selectedTargetId);
  const target = state.selectedTargetDetails;
  const settings = state.selectedTargetNotifications;
  const checks = state.selectedTargetChecks || [];
  return `
    <aside class="drawer ${open ? "open" : ""}" data-action="drawer-bg">
      <section class="drawer-panel">
        <header class="drawer-header">
          <h2 class="drawer-title">${target ? escapeHtml(target.name) : "Target"}</h2>
          <button class="button icon" data-action="close-drawer" title="Close">${icon("x")}<span class="sr-only">Close</span></button>
        </header>
        <div class="drawer-body">
          ${
            target
              ? `
                <div class="details">
                  <div class="details-row"><span class="details-key">Endpoint</span><span class="details-value">${escapeHtml(endpoint(target))}</span></div>
                  <div class="details-row"><span class="details-key">Protocol</span><span class="details-value">${escapeHtml(target.type)}</span></div>
                  <div class="details-row"><span class="details-key">Interval</span><span class="details-value">${escapeHtml(target.interval_seconds)}s</span></div>
                  <div class="details-row"><span class="details-key">Timeout</span><span class="details-value">${escapeHtml(target.timeout_ms)}ms</span></div>
                </div>
                <hr />
                <div class="toggle-row">
                  <div>
                    <strong>Email notifications</strong>
                    <div class="target-endpoint">Target #${escapeHtml(target.id)}</div>
                  </div>
                  <label class="switch">
                    <input type="checkbox" data-action="toggle-target-notifications" data-id="${target.id}" ${settings?.email_enabled !== false ? "checked" : ""} />
                    <span class="slider"></span>
                  </label>
                </div>
                <hr />
                <div class="details">
                  ${checks.length ? checks.slice(0, 8).map(checkRowMarkup).join("") : `<div class="empty">No checks</div>`}
                </div>
              `
              : `<div class="empty">Loading</div>`
          }
        </div>
      </section>
    </aside>
  `;
}

function checkRowMarkup(check) {
  return `
    <div class="details-row">
      <div>
        <strong>${escapeHtml(check.status)}</strong>
        <div class="target-endpoint">${formatDate(check.checked_at)}</div>
      </div>
      <div class="details-value">
        <span class="pill ${check.status === "up" ? "ok" : "down"}">${escapeHtml(check.protocol)}</span>
        <div class="target-endpoint">${check.latency_ms ?? "—"} ms</div>
      </div>
    </div>
  `;
}

function bindEvents() {
  document.querySelectorAll("[data-auth-mode]").forEach((button) => {
    button.addEventListener("click", () => {
      state.authMode = button.dataset.authMode;
      render();
    });
  });

  document.querySelector("[data-action='auth']")?.addEventListener("submit", onAuthSubmit);

  document.querySelectorAll("[data-view]").forEach((button) => {
    button.addEventListener("click", () => {
      state.view = button.dataset.view;
      render();
    });
  });

  document.querySelector("[data-action='logout']")?.addEventListener("click", () => {
    clearAuth();
    render();
  });

  document.querySelector("[data-action='refresh']")?.addEventListener("click", refreshAll);
  document.querySelector("[data-action='create-target']")?.addEventListener("submit", onCreateTarget);
  document.querySelector("[data-action='create-recipient']")?.addEventListener("submit", onCreateRecipient);
  document.querySelector("[data-action='test-email']")?.addEventListener("click", onTestEmail);

  document.querySelectorAll("[data-target-type] .segment").forEach((button) => {
    button.addEventListener("click", () => switchTargetType(button.dataset.type));
  });

  document.querySelectorAll("[data-action='run-check']").forEach((button) => {
    button.addEventListener("click", () => onRunCheck(button.dataset.id));
  });
  document.querySelectorAll("[data-action='open-target']").forEach((button) => {
    button.addEventListener("click", () => openTarget(button.dataset.id));
  });
  document.querySelectorAll("[data-action='delete-target']").forEach((button) => {
    button.addEventListener("click", () => onDeleteTarget(button.dataset.id));
  });
  document.querySelectorAll("[data-action='toggle-recipient']").forEach((button) => {
    button.addEventListener("click", () => onToggleRecipient(button.dataset.id, button.dataset.enabled === "true"));
  });
  document.querySelectorAll("[data-action='delete-recipient']").forEach((button) => {
    button.addEventListener("click", () => onDeleteRecipient(button.dataset.id));
  });
  document.querySelectorAll("[data-action='retry-delivery']").forEach((button) => {
    button.addEventListener("click", () => onRetryDelivery(button.dataset.id));
  });
  document.querySelector("[data-action='close-drawer']")?.addEventListener("click", closeDrawer);
  document.querySelector("[data-action='toggle-target-notifications']")?.addEventListener("change", onToggleTargetNotifications);
}

async function onAuthSubmit(event) {
  event.preventDefault();
  const form = new FormData(event.currentTarget);
  const body = {
    email: String(form.get("email") || "").trim(),
    password: String(form.get("password") || ""),
  };
  try {
    const auth = await api(`/api/v1/auth/${state.authMode === "login" ? "login" : "register"}`, {
      method: "POST",
      body,
    });
    persistAuth(auth);
    await refreshAll();
    notify("Signed in", state.session.user.email);
  } catch (error) {
    notify("Auth failed", error.message, "error");
  }
}

function switchTargetType(type) {
  const form = document.querySelector("[data-action='create-target']");
  form.querySelector("[name='type']").value = type;
  form.querySelectorAll(".segment").forEach((node) => node.classList.toggle("active", node.dataset.type === type));
  form.querySelectorAll(".http-fields").forEach((node) => {
    node.hidden = type !== "http";
    node.querySelectorAll("input,select").forEach((input) => (input.disabled = type !== "http"));
  });
  form.querySelectorAll(".tcp-fields").forEach((node) => {
    node.hidden = type !== "tcp";
    node.querySelectorAll("input").forEach((input) => (input.disabled = type !== "tcp"));
  });
}

async function onCreateTarget(event) {
  event.preventDefault();
  const form = new FormData(event.currentTarget);
  const type = String(form.get("type"));
  const body = {
    name: String(form.get("name") || "").trim(),
    type,
    interval_seconds: Number(form.get("interval_seconds")),
    timeout_ms: Number(form.get("timeout_ms")),
  };
  if (type === "http") {
    body.url = String(form.get("url") || "").trim();
    body.method = String(form.get("method") || "GET");
    body.expected_status_code = Number(form.get("expected_status_code") || 200);
  } else {
    body.host = String(form.get("host") || "").trim();
    body.port = Number(form.get("port"));
  }
  try {
    await api("/api/v1/targets", { method: "POST", body });
    notify("Target created", body.name);
    await refreshAll();
  } catch (error) {
    notify("Target failed", error.message, "error");
  }
}

async function onRunCheck(id) {
  try {
    const result = await api(`/api/v1/targets/${id}/check`, { method: "POST" });
    notify("Check saved", `Target #${id} is ${result.status}`);
    await refreshAll();
    if (state.selectedTargetId) await openTarget(state.selectedTargetId, false);
  } catch (error) {
    notify("Check failed", error.message, "error");
  }
}

async function openTarget(id, shouldRender = true) {
  state.selectedTargetId = Number(id);
  state.selectedTargetDetails = null;
  state.selectedTargetChecks = [];
  state.selectedTargetNotifications = null;
  if (shouldRender) render();
  try {
    const [target, checks, notifications] = await Promise.all([
      api(`/api/v1/targets/${id}`),
      api(`/api/v1/targets/${id}/checks`),
      api(`/api/v1/targets/${id}/notifications`),
    ]);
    state.selectedTargetDetails = target;
    state.selectedTargetChecks = checks || [];
    state.selectedTargetNotifications = notifications;
  } catch (error) {
    notify("Target failed", error.message, "error");
    closeDrawer();
  }
  render();
}

function closeDrawer() {
  state.selectedTargetId = null;
  state.selectedTargetDetails = null;
  state.selectedTargetChecks = [];
  state.selectedTargetNotifications = null;
  render();
}

async function onDeleteTarget(id) {
  if (!window.confirm(`Delete target #${id}?`)) return;
  try {
    await api(`/api/v1/targets/${id}`, { method: "DELETE" });
    notify("Target deleted", `Target #${id}`);
    await refreshAll();
  } catch (error) {
    notify("Delete failed", error.message, "error");
  }
}

async function onCreateRecipient(event) {
  event.preventDefault();
  const form = new FormData(event.currentTarget);
  const email = String(form.get("email") || "").trim();
  try {
    await api("/api/v1/notifications/recipients", { method: "POST", body: { email } });
    notify("Recipient added", email);
    await refreshAll();
  } catch (error) {
    notify("Recipient failed", error.message, "error");
  }
}

async function onToggleRecipient(id, enabled) {
  try {
    await api(`/api/v1/notifications/recipients/${id}`, {
      method: "PATCH",
      body: { is_enabled: enabled },
    });
    await refreshAll();
  } catch (error) {
    notify("Recipient failed", error.message, "error");
  }
}

async function onDeleteRecipient(id) {
  try {
    await api(`/api/v1/notifications/recipients/${id}`, { method: "DELETE" });
    await refreshAll();
  } catch (error) {
    notify("Recipient failed", error.message, "error");
  }
}

async function onTestEmail() {
  try {
    const result = await api("/api/v1/notifications/test-email", { method: "POST", body: {} });
    const count = Number(result.deliveries_count || 0);
    if (count === 0) {
      notify("No deliveries queued", "Add and enable a recipient first", "error");
    } else {
      notify("Test queued", `${count} ${count === 1 ? "delivery" : "deliveries"}`);
    }
    await refreshAll();
    if (count > 0) {
      await sleep(1200);
      await refreshAll();
    }
  } catch (error) {
    notify("Test failed", error.message, "error");
  }
}

async function onRetryDelivery(id) {
  try {
    await api(`/api/v1/notifications/deliveries/${id}/retry`, { method: "POST" });
    await refreshAll();
  } catch (error) {
    notify("Retry failed", error.message, "error");
  }
}

async function onToggleTargetNotifications(event) {
  const id = event.currentTarget.dataset.id;
  try {
    const settings = await api(`/api/v1/targets/${id}/notifications`, {
      method: "PATCH",
      body: { email_enabled: event.currentTarget.checked },
    });
    state.selectedTargetNotifications = settings;
    notify("Settings saved", `Target #${id}`);
  } catch (error) {
    notify("Settings failed", error.message, "error");
    await openTarget(id);
  }
}

boot();
