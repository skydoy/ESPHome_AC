# ESPHome_AC compatibility report Worker

Receives **anonymous, read-only** compatibility reports from devices running
[ESPHome_AC](https://github.com/skydoy/ESPHome_AC) after a successful AC identify
at boot (or when the identity changes). It never receives Wi-Fi passwords, API
keys, or raw MAC addresses.

Also serves a **short redirect** for the device **Report my AC** text sensor so the
web UI is not broken by a huge prefilled GitHub URL.

## Endpoints

### `POST /report`

`POST https://esphome-ac-report.esphome-ac-report.workers.dev/report`

```json
{
  "identity": "type=01 01 fw=Protocol 0x0215 (raw …) caps=Heat/Cool status_len=160",
  "firmware": "0.3.5",
  "board": "m5stickc-plus2",
  "device_id": "<64 hex chars: sha256 of the device MAC>",
  "status_hex": "<optional hex dump of the status frame, max 512 chars>"
}
```

| Field | Rule |
| --- | --- |
| `identity` | 8–240 chars, printable ASCII |
| `firmware` | 1–32 chars, `[A-Za-z0-9._+-]` |
| `board` | 1–48 chars, `[A-Za-z0-9._+-]` |
| `device_id` | exactly 64 lowercase hex chars |
| `status_hex` | optional, ≤ 512 hex chars |

Behaviour:

1. **Rate limit** per `device_id` and per client IP (Cloudflare Rate Limiting binding; approximate, per PoP). Raw IPs are **not** written to KV.
2. Writes/refreshes `short:<first 8 hex of device_id>` in KV for the redirect below.
3. **Dedupe** in the `REPORTS` KV namespace:
   - Same `device_id` + same identity → `200` `{ "status": "duplicate" }` (no GitHub call).
   - New identity (never seen) → open a GitHub issue labelled `compatibility`, title `[Auto] <identity>`.
   - Known identity, **new** `device_id` → comment on the existing issue (`+1` device count).
   - Known identity, same device already counted → no-op.

### `GET /r/…` — short Report my AC redirect (no issue created)

Device text sensor value (typical, ~66 chars):

`https://esphome-ac-report.esphome-ac-report.workers.dev/r/<8 hex>`

- `GET /r/<8hex>` — looks up `short:<8hex>` in KV (written on `POST /report`), then **302** to the
  prefilled GitHub `compatibility_report.yml` form.
- `GET /r?i=<identity>&f=<firmware>&b=<board>` — same **302** from query params (fallback; no KV).
  Input is validated with the same charset/size limits as `POST /report`.

Neither redirect creates a GitHub issue; they only open the new-issue form in the browser.

## Privacy

Stored in KV (no TTL on identity keys): hashed `device_id`, identity string, GitHub issue number,
a set of device ids that reported that identity, and short-code → `{identity, firmware, board}`
mappings. **IPs are not stored.**

On the device, turn the report off with the **Auto report** config switch (default ON). The short
**Report my AC** link still opens the GitHub form (via this Worker redirect).

## Deploy

```bash
export CLOUDFLARE_API_TOKEN=…          # Account API token, Edit Workers
export CLOUDFLARE_ACCOUNT_ID=48b81a3ba6ae2b514af557bcd57b715d
cd worker
npm install
npx wrangler deploy
npx wrangler secret put GITHUB_TOKEN   # fine-grained: Issues RW on skydoy/ESPHome_AC
```

The account `workers.dev` subdomain is `esphome-ac-report`.

## Local test

```bash
# Redirect only (does NOT open a GitHub issue):
curl -sS -o /dev/null -D - \
  'https://esphome-ac-report.esphome-ac-report.workers.dev/r?i=type%3DZZ%20TEST%20fw%3Dredir-test%20caps%3Dnone%20status_len%3D0&f=0.0.0-test&b=test-board'

# POST report (creates/updates an issue — use a clearly fake identity in real tests):
curl -sS -X POST https://esphome-ac-report.esphome-ac-report.workers.dev/report \
  -H 'content-type: application/json' \
  -d '{"identity":"type=TEST TEST fw=test-worker caps=none status_len=0","firmware":"0.0.0-test","board":"test-board","device_id":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}'
```
