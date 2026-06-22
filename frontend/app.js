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
  deliveryFilter: "all",
  selectedTargetId: null,
  selectedTargetDetails: null,
  selectedTargetChecks: [],
  selectedTargetNotifications: null,
  emailVerification: {
    token: new URLSearchParams(window.location.search).get("token") || "",
    status: "",
    message: "",
  },
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

function targetById(targetId) {
  return state.targets.find((target) => Number(target.id) === Number(targetId));
}

function targetStatus(target, checks = []) {
  const status = statusFor(target.id);
  const alert = activeAlertFor(target.id);
  const latestCheck = checks[0];
  const currentStatus = status?.status || latestCheck?.status;
  if (alert) return { text: "alert", tone: "critical", icon: "siren" };
  if (currentStatus === "up") return { text: "up", tone: "ok", icon: "circle-check" };
  if (currentStatus === "down") return { text: "down", tone: "down", icon: "circle-x" };
  return { text: "unknown", tone: "", icon: "circle-help" };
}

function deliveryTone(status) {
  if (status === "sent") return "sent";
  if (status === "failed") return "down";
  if (status === "retry_scheduled" || status === "sending" || status === "pending") return "pending";
  return "";
}

function targetName(targetId) {
  const target = targetById(targetId);
  return target ? target.name : `Target #${targetId}`;
}

function filteredDeliveries() {
  if (state.deliveryFilter === "all") return state.deliveries;
  return state.deliveries.filter((delivery) => delivery.status === state.deliveryFilter);
}

function countDeliveries(statuses) {
  return state.deliveries.filter((delivery) => statuses.includes(delivery.status)).length;
}

function normalizeEmail(email) {
  return String(email || "").trim().toLowerCase();
}

function accountEmail() {
  return state.session?.user?.email || "";
}

function emailVerified() {
  return state.session?.user?.email_verified !== false;
}

function accountRecipient() {
  const email = normalizeEmail(accountEmail());
  return state.recipients.find((recipient) => normalizeEmail(recipient.email) === email);
}

function legacyRecipients() {
  const email = normalizeEmail(accountEmail());
  return state.recipients.filter((recipient) => normalizeEmail(recipient.email) !== email);
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
  if (state.emailVerification.token) {
    state.emailVerification.status = "verifying";
    render();
    await verifyEmailFromLink();
    return;
  }

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
  if (state.emailVerification.status) {
    app.innerHTML = verifyEmailMarkup();
  } else if (!state.token || !state.session) {
    app.innerHTML = authMarkup();
  } else {
    app.innerHTML = shellMarkup();
  }
  bindEvents();
  renderIcons();
}

function currentViewMeta() {
  if (state.view === "target-details") {
    return {
      label: state.selectedTargetDetails?.name || "Target details",
      icon: "panel-right-open",
    };
  }
  return views.find((view) => view.id === state.view) || views[0];
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

function verifyEmailMarkup() {
  const status = state.emailVerification.status;
  const isSuccess = status === "success";
  const isError = status === "error";
  return `
    <main class="auth-layout">
      <section class="auth-panel">
        <div class="auth-head">
          <div class="brand-mark">${icon(isSuccess ? "mail-check" : isError ? "mail-x" : "loader")}</div>
          <div>
            <h1 class="auth-title">${isSuccess ? "Email verified" : isError ? "Verification failed" : "Verifying email"}</h1>
            <div class="page-subtitle">${escapeHtml(state.emailVerification.message || "Checking your verification link")}</div>
          </div>
        </div>
        <div class="verification-state ${status}">
          ${isSuccess ? icon("circle-check") : isError ? icon("triangle-alert") : icon("loader")}
          <span>${escapeHtml(state.emailVerification.message || "Please wait")}</span>
        </div>
        <button class="button primary" data-action="${state.token && state.session ? "finish-verification" : "verification-login"}">${icon(state.token && state.session ? "layout-dashboard" : "log-in", state.token && state.session ? "Open dashboard" : "Sign in")}</button>
      </section>
      <div class="notice"></div>
    </main>
  `;
}

function shellMarkup() {
  const current = currentViewMeta();
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
  if (view === "target-details") {
    const target = state.selectedTargetDetails;
    return target ? endpoint(target) : "Target checks, incidents, and notification settings.";
  }
  const labels = {
    overview: "Fleet health, active incidents, and recent delivery status.",
    targets: "Create targets, run checks, and tune per-target notifications.",
    alerts: "Active and resolved alert lifecycle events.",
    notifications: "Email recipients and delivery attempts.",
  };
  return labels[view] || "";
}

function contentMarkup() {
  if (state.view === "target-details") return targetDetailsMarkup();
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
  const healthyTargets = state.targets.length ? Math.round((upCount / state.targets.length) * 100) : 0;
  return `
    <div class="grid three">
      ${metricMarkup("Targets", state.targets.length, "radio-tower")}
      ${metricMarkup("Healthy", `${healthyTargets}%`, "activity")}
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
    <div class="grid two">
      <section class="surface">
        <div class="surface-header">
          <h2 class="surface-title">Active incidents</h2>
          <button class="button" data-view="alerts">${icon("list-tree", "Open")}</button>
        </div>
        <div class="surface-body">${activeIncidentListMarkup()}</div>
      </section>
      <section class="surface">
        <div class="surface-header">
          <h2 class="surface-title">Operations</h2>
          <button class="button" data-view="notifications">${icon("mail-check", "Notifications")}</button>
        </div>
        <div class="surface-body">
          <div class="ops-grid">
            ${operationItemMarkup("Recipients", state.recipients.length, "mail")}
            ${operationItemMarkup("Pending mail", pendingDeliveries, "clock")}
            ${operationItemMarkup("Recent deliveries", state.deliveries.length, "send")}
          </div>
        </div>
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

function operationItemMarkup(label, value, iconName) {
  return `
    <div class="operation-item">
      <div class="operation-icon">${icon(iconName)}</div>
      <div>
        <strong>${escapeHtml(value)}</strong>
        <span>${escapeHtml(label)}</span>
      </div>
    </div>
  `;
}

function activeIncidentListMarkup() {
  if (!state.activeAlerts.length) return `<div class="empty compact">No active incidents</div>`;
  return `
    <div class="event-list">
      ${state.activeAlerts
        .slice(0, 6)
        .map((alert) => {
          const target = targetById(alert.target_id);
          return `
            <button class="event-row" data-action="open-target-details" data-id="${alert.target_id}">
              <span class="event-icon">${icon("siren")}</span>
              <span>
                <strong>${escapeHtml(target?.name || `Target #${alert.target_id}`)}</strong>
                <small>${escapeHtml(alert.message || alert.type)}</small>
              </span>
              <span class="pill ${escapeHtml(alert.severity || "critical")}">${escapeHtml(alert.severity || "critical")}</span>
            </button>
          `;
        })
        .join("")}
    </div>
  `;
}

function targetsMarkup() {
  const downCount = state.targets.filter((target) => targetStatus(target).tone === "down" || targetStatus(target).tone === "critical").length;
  return `
    <div class="grid three">
      ${metricMarkup("All targets", state.targets.length, "radio-tower")}
      ${metricMarkup("Needs attention", downCount, "triangle-alert")}
      ${metricMarkup("Recipients", state.recipients.length, "mail")}
    </div>
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
        <input id="target-name" name="name" required placeholder="Main website" />
      </div>
      <div class="field protocol http-fields">
        <label for="target-url">URL</label>
        <input id="target-url" name="url" type="url" placeholder="https://example.com/health" />
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
  const health = targetStatus(target);
  return `
    <tr>
      <td>
        <div class="target-name">
          <strong>${escapeHtml(target.name)}</strong>
          <span class="target-endpoint">${escapeHtml(endpoint(target))}</span>
        </div>
      </td>
      <td><span class="pill">${escapeHtml(target.type)}</span></td>
      <td><span class="pill ${health.tone}">${icon(health.icon)}${escapeHtml(health.text)}</span></td>
      <td><span title="${escapeHtml(status?.checked_at || "")}">${escapeHtml(target.interval_seconds)}s</span></td>
      <td>
        <div class="row-actions">
          <button class="button icon" data-action="run-check" data-id="${target.id}" title="Run check">${icon("play")}<span class="sr-only">Run check</span></button>
          <button class="button icon" data-action="open-target-details" data-id="${target.id}" title="Details">${icon("panel-right-open")}<span class="sr-only">Details</span></button>
          <button class="button icon danger" data-action="delete-target" data-id="${target.id}" title="Delete">${icon("trash-2")}<span class="sr-only">Delete</span></button>
        </div>
      </td>
    </tr>
  `;
}

function alertsMarkup() {
  const rows = state.alerts.length ? state.alerts : state.activeAlerts;
  const resolvedCount = rows.filter((alert) => alert.resolved_at).length;
  return `
    <div class="grid three">
      ${metricMarkup("Active", state.activeAlerts.length, "siren")}
      ${metricMarkup("History", rows.length, "history")}
      ${metricMarkup("Resolved", resolvedCount, "circle-check")}
    </div>
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
                        <td>
                          <button class="link-button" data-action="open-target-details" data-id="${alert.target_id}">
                            ${escapeHtml(targetName(alert.target_id))}
                          </button>
                        </td>
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

function targetDetailsMarkup() {
  const target = state.selectedTargetDetails;
  const checks = state.selectedTargetChecks || [];
  const notifications = state.selectedTargetNotifications;
  if (!state.selectedTargetId || !target) {
    return `
      <section class="surface">
        <div class="surface-body"><div class="empty">Loading target</div></div>
      </section>
    `;
  }

  const health = targetStatus(target, checks);
  const relatedAlerts = state.alerts
    .concat(state.activeAlerts)
    .filter((alert, index, alerts) => {
      const sameTarget = Number(alert.target_id) === Number(target.id);
      const alertKey = alert.id ?? `${alert.target_id}:${alert.type}:${alert.created_at}:${alert.resolved_at || ""}`;
      const firstOccurrence =
        alerts.findIndex((item) => {
          const itemKey = item.id ?? `${item.target_id}:${item.type}:${item.created_at}:${item.resolved_at || ""}`;
          return itemKey === alertKey;
        }) === index;
      return sameTarget && firstOccurrence;
    })
    .slice(0, 8);
  const relatedDeliveries = state.deliveries
    .filter((delivery) => Number(delivery.target_id) === Number(target.id))
    .slice(0, 8);

  return `
    <div class="detail-toolbar">
      <button class="button" data-view="targets">${icon("arrow-left", "Targets")}</button>
      <button class="button primary" data-action="run-check" data-id="${target.id}">${icon("play", "Run check")}</button>
    </div>
    <div class="grid two detail-grid">
      <section class="surface">
        <div class="surface-body">
          <div class="status-panel ${health.tone || ""}">
            <div>
              <span class="section-label">Current status</span>
              <strong>${escapeHtml(health.text)}</strong>
              <small>${escapeHtml(endpoint(target))}</small>
            </div>
            <div class="status-icon">${icon(health.icon)}</div>
          </div>
          <div class="details">
            <div class="details-row"><span class="details-key">Protocol</span><span class="details-value">${escapeHtml(target.type)}</span></div>
            <div class="details-row"><span class="details-key">Interval</span><span class="details-value">${escapeHtml(target.interval_seconds)}s</span></div>
            <div class="details-row"><span class="details-key">Timeout</span><span class="details-value">${escapeHtml(target.timeout_ms)}ms</span></div>
            <div class="details-row"><span class="details-key">Created</span><span class="details-value">${formatDate(target.created_at)}</span></div>
          </div>
        </div>
      </section>
      <section class="surface">
        <div class="surface-header">
          <h2 class="surface-title">Notifications</h2>
          <span class="pill ${notifications?.email_enabled === false ? "" : "ok"}">${notifications?.email_enabled === false ? "muted" : "enabled"}</span>
        </div>
        <div class="surface-body">
          <div class="toggle-row">
            <div>
              <strong>Email alerts</strong>
              <div class="target-endpoint">${notifications?.email_enabled === false ? "No email will be sent for this target" : "Alert opened and resolved events will send email"}</div>
            </div>
            <label class="switch">
              <input type="checkbox" data-action="toggle-target-notifications" data-id="${target.id}" ${notifications?.email_enabled !== false ? "checked" : ""} />
              <span class="slider"></span>
            </label>
          </div>
          <div class="drawer-actions">
            <button class="button" data-view="notifications">${icon("mail-check", "Recipients")}</button>
            <button class="button danger" data-action="delete-target" data-id="${target.id}">${icon("trash-2", "Delete target")}</button>
          </div>
        </div>
      </section>
    </div>
    <div class="grid two detail-grid">
      <section class="surface">
        <div class="surface-header">
          <h2 class="surface-title">Recent checks</h2>
          <span class="pill">${checks.length}</span>
        </div>
        <div class="surface-body">${checks.length ? `<div class="details">${checks.slice(0, 12).map(checkRowMarkup).join("")}</div>` : `<div class="empty compact">No checks yet</div>`}</div>
      </section>
      <section class="surface">
        <div class="surface-header">
          <h2 class="surface-title">Incidents</h2>
          <span class="pill ${relatedAlerts.some((alert) => !alert.resolved_at) ? "critical" : ""}">${relatedAlerts.length}</span>
        </div>
        <div class="surface-body">${targetAlertsMarkup(relatedAlerts)}</div>
      </section>
    </div>
    <section class="surface">
      <div class="surface-header">
        <h2 class="surface-title">Deliveries for this target</h2>
        <button class="button" data-view="notifications">${icon("list-filter", "All deliveries")}</button>
      </div>
      <div class="surface-body">${deliveryListMarkup(relatedDeliveries)}</div>
    </section>
  `;
}

function targetAlertsMarkup(alerts) {
  if (!alerts.length) return `<div class="empty compact">No incidents for this target</div>`;
  return `
    <div class="event-list">
      ${alerts
        .map(
          (alert) => `
            <div class="event-row static">
              <span class="event-icon">${icon(alert.resolved_at ? "circle-check" : "siren")}</span>
              <span>
                <strong>${escapeHtml(alert.type)}</strong>
                <small>${escapeHtml(alert.message || "")}</small>
              </span>
              <span class="pill ${alert.resolved_at ? "ok" : "critical"}">${alert.resolved_at ? "resolved" : "active"}</span>
            </div>
          `,
        )
        .join("")}
    </div>
  `;
}

function notificationsMarkup() {
  const pendingCount = countDeliveries(["pending", "sending", "retry_scheduled"]);
  const failedCount = countDeliveries(["failed"]);
  const sentCount = countDeliveries(["sent"]);
  const visibleDeliveries = filteredDeliveries();
  const email = accountEmail();
  const recipient = accountRecipient();
  const verified = emailVerified();
  const legacy = legacyRecipients();
  const emailEnabled = Boolean(recipient?.is_enabled);
  const emailStatus = !verified ? "verify" : emailEnabled ? "enabled" : recipient ? "disabled" : "not connected";
  return `
    <div class="grid three">
      ${metricMarkup("Email channel", emailEnabled ? "On" : "Off", emailEnabled ? "mail-check" : "mail-x")}
      ${metricMarkup("Queued", pendingCount, "clock")}
      ${metricMarkup("Sent", sentCount, "send")}
    </div>
    <div class="grid two">
      <section class="surface">
        <div class="surface-header">
          <h2 class="surface-title">Account email</h2>
          <span class="pill ${emailEnabled ? "ok" : !verified ? "warning" : ""}">${escapeHtml(emailStatus)}</span>
        </div>
        <div class="surface-body">
          ${verified ? "" : verificationNoticeMarkup()}
          <form class="form" data-action="create-recipient">
            <div class="account-recipient">
              <div class="operation-icon">${icon(verified ? "mail" : "mail-warning")}</div>
              <div>
                <strong>${escapeHtml(email)}</strong>
                <span>${accountEmailStatusText(recipient, verified)}</span>
              </div>
            </div>
            <button class="button primary" type="submit" ${emailEnabled || !verified ? "disabled" : ""}>${icon(emailEnabled ? "mail-check" : "mail-plus", emailEnabled ? "Enabled" : "Enable account email")}</button>
          </form>
          ${legacy.length ? legacyRecipientsMarkup(legacy) : ""}
        </div>
        <div class="table-wrap">${recipientsTableMarkup()}</div>
      </section>
      <section class="surface">
        <div class="surface-header">
          <h2 class="surface-title">Deliveries</h2>
          <div class="toolbar">
            <button class="button" data-action="test-email">${icon("send", "Test")}</button>
          </div>
        </div>
        <div class="surface-body">
          <div class="filter-bar">
            ${deliveryFilterButton("all", "All", state.deliveries.length)}
            ${deliveryFilterButton("sent", "Sent", sentCount)}
            ${deliveryFilterButton("retry_scheduled", "Retry", countDeliveries(["retry_scheduled"]))}
            ${deliveryFilterButton("failed", "Failed", failedCount)}
          </div>
          ${deliveryListMarkup(visibleDeliveries)}
        </div>
      </section>
    </div>
  `;
}

function accountEmailStatusText(recipient, verified) {
  if (!verified) return "Verify this email before enabling alert delivery";
  if (recipient?.is_enabled) return "Alert delivery is enabled for your account email";
  if (recipient) return "Alert delivery is disabled for your account email";
  return "Alerts can be sent only to your account email";
}

function verificationNoticeMarkup() {
  return `
    <div class="notice-inline warning">
      <span class="event-icon">${icon("mail-warning")}</span>
      <div>
        <strong>Email verification required</strong>
        <span>Confirm this account email before enabling alert delivery.</span>
      </div>
      <button class="button" data-action="resend-verification-email">${icon("send", "Resend")}</button>
    </div>
  `;
}

function legacyRecipientsMarkup(recipients) {
  const enabledCount = recipients.filter((recipient) => recipient.is_enabled).length;
  return `
    <div class="notice-inline warning">
      <span class="event-icon">${icon("triangle-alert")}</span>
      <div>
        <strong>${escapeHtml(recipients.length)} legacy ${recipients.length === 1 ? "address" : "addresses"}</strong>
        <span>${enabledCount ? `${enabledCount} should be disabled before sending real alerts.` : "Disabled legacy addresses are kept only for history."}</span>
      </div>
      ${enabledCount ? `<button class="button danger" data-action="disable-legacy-recipients">${icon("bell-off", "Disable")}</button>` : ""}
    </div>
  `;
}

function deliveryFilterButton(id, label, count) {
  return `
    <button class="filter-button ${state.deliveryFilter === id ? "active" : ""}" data-action="delivery-filter" data-filter="${id}">
      <span>${escapeHtml(label)}</span>
      <strong>${escapeHtml(count)}</strong>
    </button>
  `;
}

function recipientsTableMarkup() {
  if (!state.recipients.length) return `<div class="empty">No recipients</div>`;
  return `
    <table>
      <thead><tr><th>Email</th><th>Status</th><th></th></tr></thead>
      <tbody>
        ${state.recipients
          .map((recipient) => {
            const isAccount = normalizeEmail(recipient.email) === normalizeEmail(accountEmail());
            return `
              <tr>
                <td>
                  <strong>${escapeHtml(recipient.email)}</strong>
                  ${isAccount ? "" : `<div class="target-endpoint">Legacy address</div>`}
                </td>
                <td>
                  <span class="pill ${recipient.is_enabled ? "ok" : ""}">${icon(recipient.is_enabled ? "bell" : "bell-off")}${recipient.is_enabled ? "enabled" : "disabled"}</span>
                  ${isAccount ? "" : `<span class="pill warning">legacy</span>`}
                </td>
                <td>
                  <div class="row-actions">
                    ${isAccount ? `<button class="button icon" data-action="toggle-recipient" data-id="${recipient.id}" data-enabled="${recipient.is_enabled ? "false" : "true"}" title="Toggle">${icon(recipient.is_enabled ? "bell-off" : "bell")}<span class="sr-only">Toggle</span></button>` : ""}
                    ${isAccount || recipient.is_enabled ? `<button class="button icon danger" data-action="delete-recipient" data-id="${recipient.id}" title="Disable">${icon("bell-off")}<span class="sr-only">Disable</span></button>` : ""}
                  </div>
                </td>
              </tr>
            `;
          })
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
                <strong>${escapeHtml(delivery.recipient_email || "No recipient")}</strong>
                <div class="target-endpoint">${escapeHtml(delivery.event_type)} ${delivery.target_id ? `· ${escapeHtml(targetName(delivery.target_id))}` : ""}</div>
              </div>
              <div class="details-value">
                <span class="pill ${deliveryTone(delivery.status)}">${escapeHtml(delivery.status)}</span>
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
  const open = Boolean(state.selectedTargetId) && state.view !== "target-details";
  const target = state.selectedTargetDetails;
  const settings = state.selectedTargetNotifications;
  const checks = state.selectedTargetChecks || [];
  const health = target ? targetStatus(target, checks) : null;
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
                <div class="status-panel ${health.tone || ""}">
                  <div>
                    <span class="section-label">Current status</span>
                    <strong>${escapeHtml(health.text)}</strong>
                    <small>${escapeHtml(endpoint(target))}</small>
                  </div>
                  <div class="status-icon">${icon(health.icon)}</div>
                </div>
                <div class="drawer-actions">
                  <button class="button primary" data-action="run-check" data-id="${target.id}">${icon("play", "Run check")}</button>
                  <button class="button danger" data-action="delete-target" data-id="${target.id}">${icon("trash-2", "Delete")}</button>
                </div>
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
                    <div class="target-endpoint">${settings?.email_enabled === false ? "Muted for this target" : "Enabled for this target"}</div>
                  </div>
                  <label class="switch">
                    <input type="checkbox" data-action="toggle-target-notifications" data-id="${target.id}" ${settings?.email_enabled !== false ? "checked" : ""} />
                    <span class="slider"></span>
                  </label>
                </div>
                <hr />
                <div class="section-heading">
                  <span class="section-label">Recent checks</span>
                  <span class="pill">${checks.length}</span>
                </div>
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
        <div class="target-endpoint">${formatDate(check.checked_at)}${check.error_message ? ` · ${escapeHtml(check.error_message)}` : ""}</div>
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
  document.querySelector("[data-action='finish-verification']")?.addEventListener("click", () => {
    state.emailVerification = { token: "", status: "", message: "" };
    state.view = "notifications";
    render();
  });
  document.querySelector("[data-action='verification-login']")?.addEventListener("click", () => {
    state.emailVerification = { token: "", status: "", message: "" };
    state.authMode = "login";
    render();
  });

  document.querySelector("[data-action='refresh']")?.addEventListener("click", onRefresh);
  document.querySelector("[data-action='create-target']")?.addEventListener("submit", onCreateTarget);
  document.querySelector("[data-action='create-recipient']")?.addEventListener("submit", onCreateRecipient);
  document.querySelector("[data-action='test-email']")?.addEventListener("click", onTestEmail);
  document.querySelectorAll("[data-action='delivery-filter']").forEach((button) => {
    button.addEventListener("click", () => {
      state.deliveryFilter = button.dataset.filter;
      render();
    });
  });

  document.querySelectorAll("[data-target-type] .segment").forEach((button) => {
    button.addEventListener("click", () => switchTargetType(button.dataset.type));
  });

  document.querySelectorAll("[data-action='run-check']").forEach((button) => {
    button.addEventListener("click", () => onRunCheck(button.dataset.id));
  });
  document.querySelectorAll("[data-action='open-target']").forEach((button) => {
    button.addEventListener("click", () => openTarget(button.dataset.id));
  });
  document.querySelectorAll("[data-action='open-target-details']").forEach((button) => {
    button.addEventListener("click", () => openTargetDetails(button.dataset.id));
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
  document.querySelector("[data-action='disable-legacy-recipients']")?.addEventListener("click", onDisableLegacyRecipients);
  document.querySelector("[data-action='resend-verification-email']")?.addEventListener("click", onResendVerificationEmail);
  document.querySelectorAll("[data-action='retry-delivery']").forEach((button) => {
    button.addEventListener("click", () => onRetryDelivery(button.dataset.id));
  });
  document.querySelector("[data-action='close-drawer']")?.addEventListener("click", closeDrawer);
  document.querySelectorAll("[data-action='toggle-target-notifications']").forEach((input) => {
    input.addEventListener("change", onToggleTargetNotifications);
  });
}

async function onRefresh() {
  await refreshAll();
  if (state.view === "target-details" && state.selectedTargetId) {
    await openTargetDetails(state.selectedTargetId, false);
  }
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
    notify(state.authMode === "login" ? "Signed in" : "Account created", emailVerified() ? state.session.user.email : "Check your email to verify alerts");
  } catch (error) {
    notify("Auth failed", error.message, "error");
  }
}

async function verifyEmailFromLink() {
  try {
    const session = await api("/api/v1/auth/verify-email", {
      method: "POST",
      body: { token: state.emailVerification.token },
    });
    if (state.session?.user?.id === session.user.id) {
      state.session = { ...state.session, user: session.user };
      localStorage.setItem("netwatch.session", JSON.stringify(state.session));
    }
    state.emailVerification.status = "success";
    state.emailVerification.message = "Your email is confirmed. Alerts can now be enabled.";
    window.history.replaceState({}, "", window.location.pathname);
  } catch (error) {
    state.emailVerification.status = "error";
    state.emailVerification.message = error.message;
  }
  render();
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
    if (state.view === "target-details") {
      await openTargetDetails(id, false);
    } else if (state.selectedTargetId) {
      await openTarget(state.selectedTargetId, false);
    }
  } catch (error) {
    notify("Check failed", error.message, "error");
  }
}

async function openTargetDetails(id, shouldRender = true) {
  state.view = "target-details";
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
    state.view = "targets";
  }
  render();
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
    if (state.view === "target-details") {
      state.view = "targets";
      state.selectedTargetId = null;
      state.selectedTargetDetails = null;
      state.selectedTargetChecks = [];
      state.selectedTargetNotifications = null;
    }
    await refreshAll();
  } catch (error) {
    notify("Delete failed", error.message, "error");
  }
}

async function onCreateRecipient(event) {
  event.preventDefault();
  const email = accountEmail();
  if (!emailVerified()) {
    notify("Verify email first", email, "error");
    return;
  }
  try {
    await api("/api/v1/notifications/recipients", { method: "POST", body: { email } });
    notify("Email enabled", email);
    await refreshAll();
  } catch (error) {
    notify("Email failed", error.message, "error");
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

async function onDisableLegacyRecipients() {
  const enabledLegacy = legacyRecipients().filter((recipient) => recipient.is_enabled);
  try {
    await Promise.all(enabledLegacy.map((recipient) => api(`/api/v1/notifications/recipients/${recipient.id}`, { method: "DELETE" })));
    notify("Legacy disabled", `${enabledLegacy.length} ${enabledLegacy.length === 1 ? "address" : "addresses"}`);
    await refreshAll();
  } catch (error) {
    notify("Legacy cleanup failed", error.message, "error");
  }
}

async function onResendVerificationEmail() {
  try {
    const session = await api("/api/v1/auth/resend-verification-email", {
      method: "POST",
      body: {},
    });
    state.session = { ...state.session, user: session.user };
    localStorage.setItem("netwatch.session", JSON.stringify(state.session));
    notify(
      session.user.email_verified ? "Email already verified" : "Verification sent",
      session.user.email,
    );
    await refreshAll();
  } catch (error) {
    notify("Resend failed", error.message, "error");
  }
}

async function onTestEmail() {
  if (!emailVerified()) {
    notify("Verify email first", accountEmail(), "error");
    return;
  }
  if (!accountRecipient()?.is_enabled) {
    notify("No deliveries queued", "Enable account email first", "error");
    return;
  }
  try {
    const result = await api("/api/v1/notifications/test-email", { method: "POST", body: {} });
    const count = Number(result.deliveries_count || 0);
    if (count === 0) {
      notify("No deliveries queued", "Enable account email first", "error");
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
    render();
  } catch (error) {
    notify("Settings failed", error.message, "error");
    await openTarget(id);
  }
}

boot();
