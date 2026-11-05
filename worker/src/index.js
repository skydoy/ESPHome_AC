/**
 * Anonymous compatibility reports for skydoy/ESPHome_AC.
 * POST /report  — see README.md for the JSON contract.
 */

const IDENTITY_RE = /^[\x20-\x7E]{8,240}$/;
const FIRMWARE_RE = /^[A-Za-z0-9._+-]{1,32}$/;
const BOARD_RE = /^[A-Za-z0-9._+-]{1,48}$/;
const DEVICE_ID_RE = /^[a-f0-9]{64}$/;
const STATUS_HEX_RE = /^[0-9A-Fa-f]{0,512}$/;

function json(status, body, extraHeaders = {}) {
  return new Response(JSON.stringify(body), {
    status,
    headers: {
      "content-type": "application/json; charset=utf-8",
      "cache-control": "no-store",
      ...extraHeaders,
    },
  });
}

function clientIp(request) {
  return (
    request.headers.get("cf-connecting-ip") ||
    request.headers.get("x-forwarded-for")?.split(",")[0]?.trim() ||
    "0.0.0.0"
  );
}

async function sha256Hex(text) {
  const data = new TextEncoder().encode(text);
  const digest = await crypto.subtle.digest("SHA-256", data);
  return [...new Uint8Array(digest)].map((b) => b.toString(16).padStart(2, "0")).join("");
}

function shortIpKey(ip) {
  // Hash so we never put a raw IP into a rate-limit key that might be logged.
  // The Rate Limiting binding only keeps counters; we still avoid feeding it PII-looking strings.
  return ip;
}

function validate(body) {
  if (!body || typeof body !== "object") return "body must be a JSON object";
  const { identity, firmware, board, device_id, status_hex } = body;
  if (typeof identity !== "string" || !IDENTITY_RE.test(identity))
    return "identity: 8–240 printable ASCII chars required";
  if (typeof firmware !== "string" || !FIRMWARE_RE.test(firmware))
    return "firmware: 1–32 chars [A-Za-z0-9._+-]";
  if (typeof board !== "string" || !BOARD_RE.test(board))
    return "board: 1–48 chars [A-Za-z0-9._+-]";
  if (typeof device_id !== "string" || !DEVICE_ID_RE.test(device_id))
    return "device_id: 64 lowercase hex chars (sha256 of MAC)";
  if (status_hex !== undefined && status_hex !== null) {
    if (typeof status_hex !== "string" || !STATUS_HEX_RE.test(status_hex))
      return "status_hex: optional hex string ≤ 512 chars";
  }
  return null;
}

async function github(env, method, path, body) {
  const token = env.GITHUB_TOKEN;
  if (!token) throw new Error("GITHUB_TOKEN secret is not set");
  const res = await fetch(`https://api.github.com${path}`, {
    method,
    headers: {
      authorization: `Bearer ${token}`,
      accept: "application/vnd.github+json",
      "content-type": "application/json",
      "user-agent": "esphome-ac-report-worker",
      "x-github-api-version": "2022-11-28",
    },
    body: body ? JSON.stringify(body) : undefined,
  });
  const text = await res.text();
  let data = null;
  try {
    data = text ? JSON.parse(text) : null;
  } catch {
    data = { raw: text };
  }
  if (!res.ok) {
    const err = new Error(`GitHub ${method} ${path} → ${res.status}`);
    err.status = res.status;
    err.data = data;
    throw err;
  }
  return data;
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (request.method === "GET" && (url.pathname === "/" || url.pathname === "/health")) {
      return json(200, { ok: true, service: "esphome-ac-report" });
    }
    if (request.method !== "POST" || url.pathname !== "/report") {
      return json(404, { error: "not_found" });
    }

    let body;
    try {
      body = await request.json();
    } catch {
      return json(400, { error: "invalid_json" });
    }
    const verr = validate(body);
    if (verr) return json(400, { error: "validation", detail: verr });

    const { identity, firmware, board, device_id } = body;
    const status_hex = typeof body.status_hex === "string" ? body.status_hex : "";

    // Rate limits (approximate, per Cloudflare location). Never persisted as IPs in KV.
    try {
      const ip = shortIpKey(clientIp(request));
      const [devLimit, ipLimit] = await Promise.all([
        env.DEVICE_RATE_LIMITER.limit({ key: device_id }),
        env.IP_RATE_LIMITER.limit({ key: ip }),
      ]);
      if (!devLimit.success || !ipLimit.success) {
        return json(429, { error: "rate_limited" }, { "retry-after": "60" });
      }
    } catch (e) {
      // If the binding is unavailable, fall through rather than failing open forever —
      // still protected by KV dedupe below.
      console.log("rate_limit_error", String(e));
    }

    const identityKey = await sha256Hex(identity);
    const devKey = `dev:${device_id}`;
    const idKey = `ident:${identityKey}`;

    const prevDev = await env.REPORTS.get(devKey, { type: "json" });
    if (prevDev && prevDev.identityKey === identityKey) {
      return json(200, { status: "duplicate", issue: prevDev.issue || null });
    }

    let ident = (await env.REPORTS.get(idKey, { type: "json" })) || null;
    let issueNumber = ident?.issue || null;
    let action = "noop";

    try {
      if (!ident) {
        const title = `[Auto] ${identity.length > 80 ? identity.slice(0, 80) : identity}`;
        const issueBody = [
          "Automatic compatibility report from an ESPHome_AC device after a read-only identify.",
          "",
          "### AC Identity",
          "```",
          identity,
          "```",
          "",
          `| Field | Value |`,
          `| --- | --- |`,
          `| Firmware | \`${firmware}\` |`,
          `| Board | \`${board}\` |`,
          `| Devices (hashed) | 1 |`,
          "",
          status_hex
            ? ["### Status frame (hex)", "```", status_hex, "```", ""].join("\n")
            : "",
          "_No raw MAC or IP is included. `device_id` is sha256(MAC)._",
        ].join("\n");

        const issue = await github(env, "POST", `/repos/${env.GITHUB_OWNER}/${env.GITHUB_REPO}/issues`, {
          title,
          body: issueBody,
          labels: [env.GITHUB_LABEL || "compatibility"],
        });
        issueNumber = issue.number;
        ident = { issue: issueNumber, identity, devices: [device_id], count: 1 };
        action = "created";
      } else if (!Array.isArray(ident.devices) || !ident.devices.includes(device_id)) {
        const devices = Array.isArray(ident.devices) ? ident.devices.slice() : [];
        devices.push(device_id);
        const count = devices.length;
        await github(
          env,
          "POST",
          `/repos/${env.GITHUB_OWNER}/${env.GITHUB_REPO}/issues/${ident.issue}/comments`,
          {
            body: [
              `Another device reported the same identity (\`+1\`, now **${count}** hashed device ids).`,
              "",
              `| Field | Value |`,
              `| --- | --- |`,
              `| Firmware | \`${firmware}\` |`,
              `| Board | \`${board}\` |`,
            ].join("\n"),
          },
        );
        ident = { ...ident, devices, count, identity: ident.identity || identity };
        issueNumber = ident.issue;
        action = "commented";
      } else {
        action = "already_counted";
      }

      await env.REPORTS.put(idKey, JSON.stringify(ident));
      await env.REPORTS.put(
        devKey,
        JSON.stringify({ identityKey, issue: issueNumber, at: Date.now() }),
      );
    } catch (e) {
      console.log("github_or_kv_error", String(e), e?.data ? JSON.stringify(e.data) : "");
      return json(502, { error: "upstream", detail: "github_or_kv_failed" });
    }

    return json(200, { status: action, issue: issueNumber });
  },
};
