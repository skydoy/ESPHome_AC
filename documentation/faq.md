# FAQ

## Which boards are supported?

**M5StickC Plus2 + RS485 Hat** and **ATOM Lite + ATOM Tail485**, on a Hisense (or same-bus) indoor unit whose Wi-Fi module uses the 4-pin RS-485 connector (**AEH-W4x / AEH-W41H1**). The Stick build is the one tested on a real unit. The ATOM build is not hardware-tested yet. [Compatibility](compatibility.md) · [Getting started](getting-started.md).

## Do I need YAML?

No. Use the [web installer](https://skydoy.github.io/ESPHome_AC/). YAML is optional if you adopt the device in the ESPHome Device Builder or build your own. [Configuration](configuration.md).

## Why 2.4 GHz Wi-Fi?

The installer and the quick start ask for a **2.4 GHz** network. The repository does not document a 5 GHz path for these boards.

## What is the `ESPHome_AC Setup` hotspot?

A fallback Wi-Fi access point the board opens when it has no saved network, or cannot reach it. Join it from your phone to set Wi-Fi. It is open, and only active while Wi-Fi is down. You can also use Bluetooth Improv from the Home Assistant app. [Install](install.md) · [Security](security.md).

## Can I use this commercially?

No. The project is [PolyForm Noncommercial 1.0.0](licence.md): personal and non-commercial use. Commercial use is not permitted.

## Where are the device web page screenshots?

[Device web page](web-page.md). Photos of a board inside an AC are not in the repository yet. [Open questions](open-questions.md).

## Something went wrong — where do I look?

[Troubleshooting](troubleshooting.md) · [GitHub issues](https://github.com/skydoy/ESPHome_AC/issues/new/choose)
