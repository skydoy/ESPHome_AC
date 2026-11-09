# Security and over-the-air updates

🇵🇹 [Ler em português](seguranca.md) · [Back to the README](../README.md)

## Prebuilt firmware (web installer)
The image from the [web installer](https://skydoy.github.io/ESPHome_AC/) is the same for
everyone, so it **cannot contain your keys or passwords**. It ships with:

| Item | Prebuilt firmware | Why |
|---|---|---|
| Home Assistant API encryption key | none | Any Home Assistant on your LAN can adopt it in one click. |
| OTA password (ESPHome) | none | Same public image for all users. |
| Fallback hotspot (`ESPHome_AC Setup`) | open, only while Wi-Fi is not connected | So you can set Wi-Fi from a phone. |
| Device web page (`http://<ip>/`) | **no password** (LAN only) | By design for local control and live logs. Anyone on your network can view and control the AC. |
| Firmware updates | **Firmware Update** + **Auto Update** | Downloaded over HTTPS from this project's GitHub Pages; the manifest carries an MD5 the device checks before reboot. |

On a home network you trust, this is the usual ESPHome ready-made setup. If other people or
devices share that network, harden it as below (or put the AC on a trusted VLAN).

## Factory OTA vs Device Builder
- **Web installer / factory image:** Home Assistant shows **Firmware Update**. Press
  **Install**, or leave **Auto Update** on for a night install. Your Wi-Fi is kept.
- **Adopted in the ESPHome Device Builder:** the board only *reports* a new version
  (**Update Status**, **Check for Update**). Press **Install** in the Device Builder; your
  config already follows `@stable`. Do **not** flash the generic factory image over an adopted
  build — that would wipe your API key and extras.

Details: [configuration.md → Updates](configuration.md#updates).

## Adding an API key (recommended)
Adopt the device in the ESPHome Device Builder, then add to your YAML (and Secrets):

```yaml
api:
  encryption:
    key: !secret api_key   # openssl rand -base64 32

ota:
  - platform: esphome
    password: !secret ota_password

wifi:
  ap:
    password: !secret ap_password   # at least 8 characters
```

Flash with **Install → Wirelessly**. Home Assistant will ask for the same `api_key`.
Step-by-step: [API encryption and OTA password](configuration.md#api-encryption-and-ota-password-optional).

To turn off or password-protect the device web page, see
[Web server](configuration.md#web-server).

## Reporting a vulnerability
See [SECURITY.md](../SECURITY.md). Please do **not** open a public issue for security problems.
