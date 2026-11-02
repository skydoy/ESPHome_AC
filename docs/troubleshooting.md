# Troubleshooting & FAQ

Quick fixes for the most common problems. Still stuck?
[Open an issue](https://github.com/skydoy/ESPHome_AC/issues/new/choose) and paste the log
(see [Reading the logs](#reading-the-logs)).

> ⚠️ Switch the AC off **at the breaker** before you open the indoor unit or touch any wire.

- [No AC on the screen, or the link stays down](#no-ac-on-the-screen-or-the-link-stays-down)
- [The Stick works on USB but not from the AC](#the-stick-works-on-usb-but-not-from-the-ac)
- [Home Assistant does not find the device](#home-assistant-does-not-find-the-device)
- [An update failed, or the Device Builder installs an old version](#an-update-failed-or-the-device-builder-installs-an-old-version)
- [The device's web page does not open](#the-devices-web-page-does-not-open)
- [Change the Wi-Fi network, reset or re-flash](#change-the-wi-fi-network-reset-or-re-flash)
- [Reading the logs](#reading-the-logs)

## No AC on the screen, or the link stays down
Signs: the Stick shows **No link** / `No AC: swap A/B? G0 10k pull-up?`, the ATOM LED stays
**red**, or **AC Link** is off in Home Assistant.

1. **Swap A and B.** This is the most common cause. Swapping them is safe: they carry signal only.
2. **Check GND.** The plug's GND wire must go to the GND terminal (Hat terminal 4, Tail485 GND).
3. **Check the power.** The board must stay on with USB unplugged. 5 V plug: X goes to the
   board's **5V header pin**, never to the 12V terminal. 12 V plug: X goes to the 12V terminal.
   **Reset Reason** showing `brownout` means a weak or loose 5 V supply.
4. **Not sure which wire is which?** See [Wiring → Measure the plug](wiring.md#1-measure-the-plug).

The link normally comes up within a minute of power-on.

## The Stick works on USB but not from the AC
The Stick's TX line is **G0**, a boot pin. Powered from the AC, some setups stay in download
mode: blank screen, no Wi-Fi. Fix: solder a **10 kΩ resistor from G0 to 3.3 V** on the Stick
header. [Details](wiring.md#3-boot-pin-g0-read-this-before-closing-the-ac). The ATOM is not affected.

## Home Assistant does not find the device
1. Check that the device is on Wi-Fi: the Stick shows its IP address at the bottom right (when
   the AC link is up), the ATOM LED is not **blue**.
2. Add it by hand: **Settings → Devices & services → Add integration → ESPHome**, enter the
   device's IP address, port `6053`.
3. Home Assistant and the device must be on the same network. Guest Wi-Fi and "client
   isolation" block discovery.
4. Shows as unavailable after you added an API key, or after installing the web-installer firmware
   over your own build? See [API encryption](configuration.md#api-encryption-and-ota-password-optional).

## An update failed, or the Device Builder installs an old version
- **Web-installer firmware:** updates install at night by themselves (**Auto Update** switch),
  or press **Install** on the **Firmware Update** entity. An update takes 1-2 minutes, and the
  device looks offline until it reboots. If it fails, the old firmware keeps running; try
  again later.
- **Adopted in the ESPHome Device Builder:** your YAML must import `@stable`:
  ```yaml
  packages:
    skydoy.esphome-ac: github://skydoy/ESPHome_AC/devices/hisense-m5stickc-plus2.yaml@stable
  ```
  (ATOM: `devices/hisense-atom-lite-tail485.yaml@stable`). The Device Builder caches GitHub
  for up to a day, so a new release can take a day to appear. To get it now, open the device's
  **⋮** menu → **Clean Build Files**, then **Install**.
- **Errors such as "invalid option" after an update:** **Clean Build Files**, then **Install**.
- **Never press *Install* on a "Firmware Update" entity of an adopted device** (only builds
  adopted before v0.2.9 have one): it would replace your config. Press **Install** in the Device
  Builder once and the entity disappears.

## The device's web page does not open
Type **`http://`** followed by the IP address, for example `http://192.168.1.50/`. The device has
no `https://`, and some browsers add it by themselves. The page needs internet access in the
browser (it loads its scripts from the ESPHome website).

## Change the Wi-Fi network, reset or re-flash
- **New Wi-Fi network:** if the device cannot reach its old network, it opens the
  `Hisense-AC Setup` hotspot after about a minute. Join it with your phone and pick the new
  network. Or connect it by USB, open the [web installer](https://skydoy.github.io/ESPHome_AC/),
  click **Connect** and choose **Change Wi-Fi**.
- **Factory reset:** ATOM: hold the button for 10 s (the LED turns white, then it reboots into
  setup mode). Stick, or either board: connect by USB, **Connect** in the web installer and
  choose **Erase User Data** (shown when it already runs the latest version).
- **Re-flash:** **Connect** in the web installer. On a board that already runs this firmware,
  **Update** keeps your Wi-Fi settings. A first install erases the board. ATOM: unplug the
  Tail485 first, it uses the USB-C port.

## Reading the logs
- **In a browser:** open `http://<device-ip>/`; the page shows the live log.
- **Over USB:** **Connect** in the [web installer](https://skydoy.github.io/ESPHome_AC/) →
  **Logs & Console**.
- **ESPHome Device Builder:** the device's **Logs** button.

Useful lines: `hisense_ac` (link and frames), `auto_update` (night updates), `update_check`
(new release found). To capture raw AC frames for a bug report, turn on **AC Frame Logging**
([details](configuration.md#raw-frame-logging)).
