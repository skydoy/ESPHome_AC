#!/usr/bin/env python3
"""Firmware paths that may cut a release.

A push to main creates a release only when at least one changed path is compiled
into a device image. Markdown, docs, the web installer, and CHANGELOG are not.

  python3 tools/firmware_paths.py --changed <before-sha> <after-sha>
  python3 tools/firmware_paths.py --self-test

Exit status of --changed: 0 firmware changed, 1 none, 2 usage or git error.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

# Compiled into the image:
#   devices/*.yaml          device, factory, and setup configs CI builds
#   core/**                 packages, screen assets, web-ui.js (js_include)
#   components/**           external components, including setup_page.html
#   brands/**/*.yaml        brand package pulled in by the device configs
#   secrets.yaml[.example|.template|.sample]
#                           ESPHome substitutes secrets.yaml into the config.
#                           None of these is committed today (secrets.yaml is
#                           gitignored); the names are here so a template counts.
#
# Not firmware: documentation/, docs/, README*, CHANGELOG.md, static/ (the web
# installer page), examples/, blueprints/, adapters/, brands/**/*.md, worker/,
# tests/, tools/, .github/, mkdocs, overrides/. There is no packages/ directory;
# shared packages live in core/ and brands/. Any *.md path is ignored, including
# one added later under core/ or components/.
FIRMWARE_RE = re.compile(
    r"(?:devices/.+\.ya?ml"
    r"|core/.+"
    r"|components/.+"
    r"|brands/.+\.ya?ml"
    r"|secrets\.yaml"
    r"|secrets\.yaml\.(?:example|template|sample))"
)
SHA_RE = re.compile(r"^[0-9a-fA-F]{40}$")

# Globs listed on auto-release.yml's push trigger. The in-workflow check is
# stricter (it drops *.md); these globs only decide whether the job starts.
WORKFLOW_PATH_GLOBS = (
    "devices/**/*.yaml",
    "devices/**/*.yml",
    "core/**",
    "components/**",
    "brands/**/*.yaml",
    "brands/**/*.yml",
    "secrets.yaml",
    "secrets.yaml.example",
    "secrets.yaml.template",
    "secrets.yaml.sample",
)


def is_firmware(path: str) -> bool:
    """True when this repo-relative path is compiled into a device image."""
    rel = path.replace("\\", "/").lstrip("./")
    if rel.endswith(".md"):
        return False
    return FIRMWARE_RE.fullmatch(rel) is not None


def changed_firmware(before: str, after: str) -> list[str]:
    if not SHA_RE.fullmatch(before) or not SHA_RE.fullmatch(after):
        sys.stderr.write("before and after must be 40-character SHAs\n")
        raise SystemExit(2)
    try:
        out = subprocess.check_output(
            ["git", "diff", "--name-only", before, after],
            text=True,
            stderr=subprocess.PIPE,
        )
    except subprocess.CalledProcessError as exc:
        sys.stderr.write(exc.stderr or str(exc))
        raise SystemExit(2) from exc
    return [line for line in out.splitlines() if line and is_firmware(line)]


def _self_test() -> None:
    firmware = [
        "devices/hisense-m5stickc-plus2.yaml",
        "devices/esphome-ac-m5stickc-plus2.factory.yaml",
        "devices/setup-m5stickc-plus2.yaml",
        "core/web-ui.js",
        "core/screen/boot_logo.png",
        "core/boards/m5stickc-plus2.yaml",
        "components/hisense_ac/hisense_ac.cpp",
        "components/ac_setup/setup_page.html",
        "components/hisense_ac/climate.py",
        "brands/hisense/hisense-ac.yaml",
        "secrets.yaml",
        "secrets.yaml.example",
        "secrets.yaml.template",
        "secrets.yaml.sample",
    ]
    not_firmware = [
        "README.md",
        "README.pt.md",
        "CHANGELOG.md",
        "CONTRIBUTING.md",
        "documentation/configuration.md",
        "documentation/configuration.pt.md",
        "docs/configuration.md",
        "docs/overview.md",
        "static/index.html",
        "static/wiring.svg",
        "examples/my-ac-livingroom.yaml",
        "blueprints/automation/esphome_ac_update_available.yaml",
        "adapters/README.md",
        "brands/hisense/README.md",
        "brands/hisense/hardware.md",
        "brands/hisense/protocol.md",
        "core/README.md",
        "components/hisense_ac/README.md",
        "worker/src/index.js",
        "worker/wrangler.toml",
        "tests/test_protocol.cpp",
        "tools/check_flash_budget.py",
        ".github/workflows/publish.yml",
        ".github/workflows/auto-release.yml",
        "mkdocs.yml",
        "requirements-docs.txt",
        "devices/.gitignore",
        "LICENSE",
    ]
    for path in firmware:
        if not is_firmware(path):
            raise SystemExit(f"expected firmware: {path}")
    for path in not_firmware:
        if is_firmware(path):
            raise SystemExit(f"expected non-firmware: {path}")
    workflow = Path(".github/workflows/auto-release.yml")
    if workflow.is_file():
        text = workflow.read_text(encoding="utf-8")
        for glob in WORKFLOW_PATH_GLOBS:
            if f'"{glob}"' not in text and f"'{glob}'" not in text:
                raise SystemExit(f"auto-release.yml is missing path glob {glob}")
    print("firmware path self-test passed")


def main(argv: list[str]) -> int:
    if len(argv) == 2 and argv[1] == "--self-test":
        _self_test()
        return 0
    if len(argv) == 4 and argv[1] == "--changed":
        found = changed_firmware(argv[2], argv[3])
        if not found:
            print("No firmware files changed.")
            return 1
        print("Firmware files changed:")
        for path in found:
            print(f"  {path}")
        return 0
    sys.stderr.write(
        "usage: firmware_paths.py --self-test | --changed <before-sha> <after-sha>\n"
    )
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
