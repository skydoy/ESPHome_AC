# Updates

The board checks for new firmware at start-up and every 6 hours. Only tested releases from the `stable` branch are offered. Wi-Fi and settings are kept on a factory update.

## Automatic (factory / web-installer image)

Home Assistant shows **Firmware Update**. Press **Install**, or leave **Auto Update** on (configuration, default on, kept across reboots).

With **Auto Update** on, the device checks once a night at **03:MM** local time. The minute is random, so devices do not all hit GitHub at once. It never runs in the first 10 minutes after boot and logs every decision under the `auto_update` tag. Switch it off to update by hand.

An update takes about 1–2 minutes. The download blocks the device's main loop, so Home Assistant shows no progress and the device looks offline until it reboots. The Stick's screen shows a progress bar. The live log shows `Progress: NN%` until the update starts.

Manifests:

| Who | Manifest |
|---|---|
| Stick, flashed before v0.3.0 (legacy `hisense-ac`) | `https://skydoy.github.io/ESPHome_AC/firmware/manifest.json` |
| ATOM, flashed before v0.3.0 | `https://skydoy.github.io/ESPHome_AC/firmware/atom-lite-tail485/manifest.json` |
| Stick, new install (v0.3.0+, `esphome-ac`) | `https://skydoy.github.io/ESPHome_AC/install/m5stickc-plus2/manifest.json` |
| ATOM, new install (v0.3.0+) | `https://skydoy.github.io/ESPHome_AC/install/atom-lite-tail485/manifest.json` |

Legacy manifests keep serving `hisense-ac` builds. New installs use `esphome-ac` names and the friendly name **Air Conditioner**. Adopted Device Builder configs set `name` and `friendly_name` themselves. The base configs still default to `hisense-ac` / `Hisense AC`.

## Device Builder (adopted)

The board only *reports* a new version (**Update Available**, **Latest Version**, **Check for Update**, **Update Status**). Nothing is installed. Press **Install** → *Wirelessly* in the Device Builder. Your YAML imports `@stable`, so you do not edit it.

```yaml
packages:
  skydoy.esphome-ac: github://skydoy/ESPHome_AC/devices/hisense-m5stickc-plus2.yaml@stable
  # ATOM Lite: devices/hisense-atom-lite-tail485.yaml
```

The `stable` branch is moved by CI to every release, so **Install** builds the latest release. The `hisense_ac` component inside the package is pinned to that release's tag.

The Device Builder caches git sources for up to 1 day. Open the device's **⋮** menu → **Clean Build Files**, then **Install**, to fetch a release now.

Want a fixed version? Replace `@stable` with a tag such as `@v0.2.12`. Avoid `@main` (development only). It can mix new YAML with an old cached component.

Adopted before v0.2.9? Those builds still include the update entity. Do not press *Install* on it. Press **Install** in the Device Builder once and the entity disappears.

The adopted build has no **Firmware Update** entity on purpose. The generic factory image would replace your config and drop your API key.

!!! warning "Do not flash the factory image over an adopted build"
    That wipes your API key and extras. See [Security](security.md).

## Notify me

Adopted builds can import the blueprint on [Home Assistant → Update notification](home-assistant.md#update-notification).

Full reference, including what the **Firmware Version** sensor shows: [Configuration → Updates](configuration.md#updates).
