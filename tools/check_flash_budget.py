#!/usr/bin/env python3
"""Fail the build when a factory/OTA app image exceeds 90% of its app partition.

4 MB boards use a 1,835,008-byte app slot. The M5StickC Plus2 (8 MB) has its own,
larger slot. The size is read from the build's partitions.csv (the ota_0 app
partition). firmware.factory.bin is a merged image (bootloader + partition table
+ app); the bytes that occupy the app slot are firmware.bin and firmware.ota.bin,
and both are checked.

Usage: tools/check_flash_budget.py path/to/device.factory.yaml
"""

from __future__ import annotations

import sys
from pathlib import Path

# size / slot > 0.90 fails. Equal to 90% still passes.
LIMIT_NUM = 9
LIMIT_DEN = 10
SLOT_4MB = 1_835_008


def device_name(yaml: Path) -> str:
    """The esphome name this config builds, from its substitutions or esphome: block."""
    in_sub = False
    for line in yaml.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if stripped.startswith("substitutions:"):
            in_sub = True
            continue
        if in_sub and stripped and not stripped.startswith("#") and not line[:1].isspace():
            in_sub = False
        if not stripped.startswith("name:") or stripped.startswith("name_add"):
            continue
        value = stripped.split(":", 1)[1].strip().strip("\"'")
        if value and not value.startswith(("$", "!", "{")):
            return value.replace("_", "-")
    return ""


def app_slot(partitions: Path) -> int:
    slot = 0
    for raw in partitions.read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        parts = [p.strip() for p in line.split(",")]
        if len(parts) < 5 or parts[1] != "app":
            continue
        size = int(parts[4], 0)
        slot = size
        if parts[2] == "ota_0":
            return size
    if slot == 0:
        raise SystemExit(f"no app partition in {partitions}")
    return slot


def find_build(yaml: Path, name: str) -> Path:
    roots = [
        yaml.parent / ".esphome" / "build",
        yaml.parent.parent / ".esphome" / "build",
    ]
    for root in roots:
        cand = root / name
        if cand.is_dir():
            return cand
    found = [p for root in roots if root.is_dir() for p in root.iterdir() if p.is_dir()]
    if len(found) == 1:
        return found[0]
    if found and not name:
        # Name comes from a package (legacy configs): use the build that just ran.
        return max(found, key=lambda p: p.stat().st_mtime)
    raise SystemExit(f"build directory for {name!r} not found (looked in {roots})")


def app_images(build: Path) -> list[Path]:
    found: list[Path] = []
    seen: set[Path] = set()
    for path in build.rglob("firmware*.bin"):
        if "factory" in path.name:
            continue
        if path.name not in ("firmware.bin", "firmware.ota.bin"):
            continue
        resolved = path.resolve()
        if resolved in seen:
            continue
        seen.add(resolved)
        found.append(path)
    return found


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_flash_budget.py <device.yaml>", file=sys.stderr)
        return 2
    yaml = Path(sys.argv[1]).resolve()
    name = device_name(yaml)
    build = find_build(yaml, name)
    tables = list(build.rglob("partitions.csv"))
    if not tables:
        print(f"no partitions.csv under {build}", file=sys.stderr)
        return 1
    slot = app_slot(tables[0])
    images = app_images(build)
    if not images:
        print(f"no firmware.bin / firmware.ota.bin under {build}", file=sys.stderr)
        return 1
    print(f"{yaml.name}: app partition {slot} bytes (90% is {slot * LIMIT_NUM // LIMIT_DEN})")
    if slot == SLOT_4MB:
        print("  slot: 4 MB board (1,835,008 bytes)")
    else:
        print(f"  slot: {slot} bytes (Plus2 / non-4MB board uses its own app partition)")
    failed = False
    for image in images:
        size = image.stat().st_size
        pct = 100.0 * size / slot
        print(f"  {image.relative_to(build)}: {size} bytes ({pct:.1f}%)")
        if size * LIMIT_DEN > slot * LIMIT_NUM:
            print(
                f"::error::{image.name} is {size} bytes, over 90% of the {slot}-byte app partition",
                file=sys.stderr,
            )
            failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
