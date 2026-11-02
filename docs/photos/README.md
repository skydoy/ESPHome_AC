# Photos for the web installer

The [web installer](https://skydoy.github.io/ESPHome_AC/) has a slot for each photo below. A slot
stays invisible until its file exists, so add photos one at a time, in any order.

**Where:** save each file in [`static/`](../../static) with the exact name below, commit, and
publish a release (the installer page is rebuilt from `static/` on every release).

**How:** real photos of real hardware only (no renders or AI images). JPEG, landscape, about
1600 px wide and under 300 KB. Good light, no flash glare, wire colours and terminal labels
readable. Switch the AC off at the breaker before opening the unit for the photo.

| File | Shows | Installer step |
|---|---|---|
| `photo-ac-plug.jpg` | The AC's 4-wire Wi-Fi module plug, unplugged from the original module, close up, wire colours visible | 4. Wire it to the AC (both boards) |
| `photo-stick-wiring.jpg` | RS485 Hat terminal block with the A, B and GND wires screwed in (terminals 2, 1, 4) and the X wire where your plug needs it (Stick 5V header pin, or terminal 3 for 12 V); terminal numbers readable | 4. Wire it to the AC (Stick) |
| `photo-stick-installed.jpg` | The M5StickC Plus2 + Hat fitted inside the indoor unit, cover open, screen on and showing a green **OK** | 4. Wire it to the AC (Stick) |
| `photo-atom-wiring.jpg` | ATOM Tail485 terminal block with A, B and GND screwed in and the X wire where your plug needs it (ATOM 5V header pin, or the Tail485 12V terminal for 12 V) | 4. Wire it to the AC (ATOM) |
| `photo-atom-installed.jpg` | The ATOM Lite + Tail485 fitted inside the indoor unit, LED green | 4. Wire it to the AC (ATOM) |

Wiring diagrams (`wiring.svg`, `wiring-atom-lite.svg`) are already on the page; the photos
complement them, they do not replace them. The [quick start](../quick-start.md) and
[guia rápido](../guia-rapido.md) have a hidden marker where each photo goes; replace it with the
image once the file exists.
