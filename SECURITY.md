# Security policy

## Supported versions
Only the latest release receives fixes. Devices update themselves over the air through the
**Firmware Update** entity.

| Version | Supported |
|---|---|
| latest `0.2.x` | ✅ |
| older | ❌ |

## Reporting a vulnerability
Please **do not open a public issue**. Report it privately through
[GitHub private vulnerability reporting](https://github.com/skydoy/ESPHome_AC/security/advisories/new).
Include the firmware version, the affected file or feature, and steps to reproduce.

You can expect a first reply within 7 days. Once a fix is released, the advisory is published
with credit to the reporter unless you prefer otherwise.

## Scope
In scope: the firmware in this repository, the release workflow and the web installer.
Out of scope: ESPHome itself, Home Assistant and the AC's own electronics (report those
upstream). Remember that the AC indoor unit has mains voltage inside; this is a safety
matter, not a software vulnerability.

## Hardening tips
The prebuilt firmware ships without an API key, OTA password or hotspot password so that any
Home Assistant can adopt it. After adopting it in the ESPHome dashboard, add these to your own
configuration and flash it once:
- `api: encryption: key:` (Home Assistant asks for it when you add the device)
- an OTA password (`ota: - platform: esphome password:`)
- a fallback hotspot password (`wifi: ap: password:`)
