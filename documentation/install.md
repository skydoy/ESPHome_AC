# Install

Do this **before** you open the air conditioner.

## 1. Open the installer

Open **[https://skydoy.github.io/ESPHome_AC/](https://skydoy.github.io/ESPHome_AC/)** in **Chrome or Edge** on a computer. The page has to be served over https. These pages do not change the installer.

## 2. Pick your board

- **ATOM Lite + Tail485** — manifest `install/atom-lite-tail485/manifest.json`
- **M5StickC Plus2 + RS485 Hat** — manifest `install/m5stickc-plus2/manifest.json` (selected by default on the installer)

Hisense AEH-W4x / AEH-W41H1 only. Other brands are not in the installer.

## 3. Connect USB and Install

Use a USB-C **data** cable. On the ATOM, flash **before** you fit the Tail485: the tail uses the same USB-C port.

Press **Install**, pick the serial port, then confirm.

You should see a progress bar (about 1 minute) and *Installation complete*. The same window then asks for Wi-Fi. Pick your **2.4 GHz** network and type the password, or later join the `ESPHome_AC Setup` hotspot from your phone. You can also set Wi-Fi over Bluetooth (Improv) from the Home Assistant app.

The hotspot comes back on its own whenever the board cannot reach the saved Wi-Fi. It is open, and only while Wi-Fi is not connected.

!!! tip "Board not showing up?"
    Try another cable. Stick: install the CH9102 driver ([M5Stack](https://docs.m5stack.com/en/core/M5StickC%20PLUS2)). ATOM: install the [FTDI VCP driver](https://ftdichip.com/drivers/vcp-drivers/).

After Wi-Fi joins you should see:

- **Stick:** the screen stops saying `Wi-Fi: join "ESPHome_AC Setup"`. *No AC* is normal until it is wired. With Wi-Fi up and no AC link, the IP address is at the bottom right for about 60 seconds. After that the same line becomes `No AC: swap A/B? G0 10k pull-up?` and the address is no longer drawn. That hint means the board is on Wi-Fi and has not heard from an AC yet. It is expected before you wire it. To read the address, look during that first minute, or restart the board and look again before 60 seconds. Home Assistant can still discover the device without that address. The USB log is **Connect → Logs & Console** in the installer.
- **ATOM:** the LED turns from **blue** to **red** until the AC link is up. Red with Wi-Fi and no wires is expected.

## Next

[Wiring](wiring.md) → connect the plug with the breaker off, then [Home Assistant](home-assistant.md).
