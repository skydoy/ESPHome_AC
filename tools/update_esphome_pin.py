#!/usr/bin/env python3
"""Bump the pinned ESPHome release and the firmware patch version.

Used by .github/workflows/update-esphome.yml. Does not commit, push, or open a
pull request. Stable releases are YYYY.M.P from PyPI (GitHub releases as a
fallback). Pre-releases, betas, and yanked versions are ignored.

  python3 tools/update_esphome_pin.py --latest-only
  python3 tools/update_esphome_pin.py --plan --latest 2026.11.0 --tags-file -
  python3 tools/update_esphome_pin.py --apply --latest 2026.11.0 --date 2026-11-24
  python3 tools/update_esphome_pin.py --self-test
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

STABLE_RE = re.compile(r"\d{4}\.\d{1,2}\.\d+")
PROJECT_RE = re.compile(r"\d+\.\d+\.\d+")
PIN_LINE_RE = re.compile(r'^\s*version:\s*"(\d{4}\.\d{1,2}\.\d+)"\s*$', re.M)
PROJECT_LINE_RE = re.compile(r'^\s*project_version:\s*"([^"]+)"\s*$', re.M)
USER_AGENT = "ESPHome_AC-update-esphome"

DEVICE_VERSION_FILES = (
    "devices/hisense-m5stickc-plus2.yaml",
    "devices/hisense-atom-lite-tail485.yaml",
    "devices/hisense-m5stickc-plus.yaml",
)
EXAMPLE_FILE = "examples/my-ac-livingroom.yaml"
PUBLISH_WORKFLOW = ".github/workflows/publish.yml"
CHANGELOG = "CHANGELOG.md"
# CHANGELOG history stays as written. This file embeds the current pin in its tests;
# rewriting it would make the bump commit edit the updater.
SKIP_PIN_FILES = {CHANGELOG, "tools/update_esphome_pin.py"}


def version_key(version: str) -> tuple[int, ...]:
    return tuple(int(part) for part in version.split("."))


def bounded(version: str) -> re.Pattern[str]:
    # Don't treat a following sentence period as part of the version. A longer
    # number such as 2026.10.10 still fails the lookahead because a digit follows.
    return re.compile(rf"(?<![\d.]){re.escape(version)}(?!\d)")


def fetch_json(url: str) -> dict:
    req = urllib.request.Request(url, headers={"Accept": "application/json", "User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=60) as resp:
        return json.load(resp)


def latest_from_pypi(payload: dict) -> str:
    stables: list[str] = []
    for version, files in payload.get("releases", {}).items():
        if not STABLE_RE.fullmatch(version):
            continue
        if not files or all(item.get("yanked") for item in files):
            continue
        stables.append(version)
    if not stables:
        raise SystemExit("PyPI has no installable stable esphome release")
    return max(stables, key=version_key)


def latest_from_github(payload: dict) -> str:
    if payload.get("draft") or payload.get("prerelease"):
        raise SystemExit("GitHub latest ESPHome release is not stable")
    tag = str(payload.get("tag_name", ""))
    if tag.startswith("v"):
        tag = tag[1:]
    if not STABLE_RE.fullmatch(tag):
        raise SystemExit(f"GitHub tag {tag!r} is not a stable YYYY.M.P release")
    return tag


def fetch_latest() -> str:
    errors: list[str] = []
    try:
        return latest_from_pypi(fetch_json("https://pypi.org/pypi/esphome/json"))
    except Exception as exc:  # noqa: BLE001 - fall back to GitHub releases
        errors.append(f"PyPI: {exc}")
    try:
        return latest_from_github(
            fetch_json("https://api.github.com/repos/esphome/esphome/releases/latest")
        )
    except Exception as exc:  # noqa: BLE001
        errors.append(f"GitHub: {exc}")
    raise SystemExit("could not find a stable ESPHome release\n" + "\n".join(errors))


def parse_tags(raw: str) -> set[str]:
    tags: set[str] = set()
    for line in raw.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        # git ls-remote: "<sha>\trefs/tags/v0.8.0"
        if "\t" in line:
            line = line.split("\t", 1)[1].strip()
        if line.startswith("refs/tags/"):
            line = line[len("refs/tags/") :]
        if line.endswith("^{}"):
            continue
        tags.add(line)
    return tags


def bump_patch(version: str) -> str:
    match = PROJECT_RE.fullmatch(version)
    if not match:
        raise SystemExit(f"project_version {version!r} is not x.y.z")
    major, minor, patch = (int(part) for part in version.split("."))
    return f"{major}.{minor}.{patch + 1}"


def next_project_version(current: str, tags: set[str]) -> str:
    version = bump_patch(current)
    # Skip a patch that was already tagged (do not release the same version twice).
    for _ in range(100):
        if f"v{version}" not in tags:
            return version
        version = bump_patch(version)
    raise SystemExit("could not find a free patch version")


def tracked_files(root: Path) -> list[Path]:
    try:
        out = subprocess.check_output(["git", "ls-files", "-z"], cwd=root, stderr=subprocess.DEVNULL)
    except (subprocess.CalledProcessError, FileNotFoundError):
        return [path for path in root.rglob("*") if path.is_file() and ".git" not in path.parts]
    files: list[Path] = []
    for raw in out.split(b"\0"):
        if not raw:
            continue
        path = root / raw.decode()
        if path.is_file():
            files.append(path)
    return files


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def current_pin(root: Path) -> str:
    text = read_text(root / PUBLISH_WORKFLOW)
    found = PIN_LINE_RE.findall(text)
    if len(found) != 1:
        raise SystemExit(f"expected one ESPHome version pin in {PUBLISH_WORKFLOW}, found {found}")
    pin = found[0]
    for path in (root / ".github" / "workflows").glob("*.yml"):
        for match in re.findall(r"esphome==(\d{4}\.\d{1,2}\.\d+)", read_text(path)):
            if match != pin:
                raise SystemExit(f"{path.relative_to(root)} pins esphome=={match}, publish.yml pins {pin}")
        for match in PIN_LINE_RE.findall(read_text(path)):
            if match != pin:
                raise SystemExit(f"{path.relative_to(root)} version pin {match} != {pin}")
    return pin


def current_project_version(root: Path) -> str:
    found: list[str] = []
    for rel in DEVICE_VERSION_FILES:
        versions = PROJECT_LINE_RE.findall(read_text(root / rel))
        if len(versions) != 1:
            raise SystemExit(f"expected one project_version in {rel}, found {versions}")
        found.append(versions[0])
    if len(set(found)) != 1:
        raise SystemExit(f"project_version differs across device configs: {found}")
    if not PROJECT_RE.fullmatch(found[0]):
        raise SystemExit(f"project_version {found[0]!r} is not x.y.z")
    return found[0]


def pin_files(root: Path, pin: str) -> list[Path]:
    pattern = bounded(pin)
    found: list[Path] = []
    for path in tracked_files(root):
        rel = path.relative_to(root).as_posix()
        if rel in SKIP_PIN_FILES:
            continue
        try:
            text = read_text(path)
        except UnicodeDecodeError:
            continue
        if pattern.search(text):
            found.append(path)
    return found


def replace_pin(text: str, old: str, new: str) -> tuple[str, int]:
    return bounded(old).subn(new, text)


def replace_project_version(text: str, old: str, new: str) -> str:
    pattern = re.compile(rf'^(\s*project_version:\s*"){re.escape(old)}("\s*)$', re.M)
    updated, count = pattern.subn(rf"\g<1>{new}\2", text)
    if count != 1:
        raise SystemExit(f"expected to replace one project_version {old}, replaced {count}")
    return updated


def replace_example_ref(text: str, old: str, new: str) -> str:
    pattern = re.compile(rf"^(\s*ref:\s*)v{re.escape(old)}\b", re.M)
    updated, count = pattern.subn(rf"\g<1>v{new}", text)
    if count != 1:
        raise SystemExit(f"expected to replace one example ref v{old}, replaced {count}")
    return updated


def update_changelog(text: str, old: str, new: str, esphome: str, day: str) -> str:
    match = re.search(r"## \[Unreleased\]\n(?P<body>.*?)(?=\n## \[)", text, re.S)
    if not match:
        raise SystemExit("CHANGELOG.md has no ## [Unreleased] section followed by a version")
    body = match.group("body").strip("\n")
    notes = f"{body}\n\n" if body else ""
    section = f"\n## [{new}] - {day}\n\n{notes}- Updated ESPHome to {esphome}.\n"
    rest = text[match.end() :]
    if not rest.startswith("\n"):
        rest = "\n" + rest
    text = text[: match.start()] + "## [Unreleased]\n" + section + rest
    old_link = f"[Unreleased]: https://github.com/skydoy/ESPHome_AC/compare/v{old}...HEAD"
    new_link = f"[Unreleased]: https://github.com/skydoy/ESPHome_AC/compare/v{new}...HEAD"
    if old_link not in text:
        raise SystemExit(f"CHANGELOG.md Unreleased link does not point at v{old}")
    text = text.replace(old_link, new_link, 1)
    anchor = f"[{old}]:"
    pos = text.find(anchor)
    if pos < 0:
        raise SystemExit(f"CHANGELOG.md has no link for {old}")
    compare = f"[{new}]: https://github.com/skydoy/ESPHome_AC/compare/v{old}...v{new}\n"
    return text[:pos] + compare + text[pos:]


def plan_lines(skip: bool, esphome: str, old_esphome: str, project: str, old_project: str) -> str:
    branch = f"deps/esphome-{esphome}"
    return "\n".join(
        [
            f"skip={'true' if skip else 'false'}",
            f"esphome={esphome}",
            f"old_esphome={old_esphome}",
            f"project_version={project}",
            f"old_project_version={old_project}",
            f"branch={branch}",
        ]
    )


def build_plan(root: Path, latest: str, tags: set[str]) -> dict[str, str]:
    if not STABLE_RE.fullmatch(latest):
        raise SystemExit(f"refusing non-stable ESPHome version {latest!r}")
    pin = current_pin(root)
    if version_key(latest) < version_key(pin):
        raise SystemExit(f"latest ESPHome {latest} is older than the pin {pin}")
    project = current_project_version(root)
    if latest == pin:
        return {
            "skip": "true",
            "esphome": latest,
            "old_esphome": pin,
            "project_version": project,
            "old_project_version": project,
            "branch": f"deps/esphome-{latest}",
        }
    nxt = next_project_version(project, tags)
    return {
        "skip": "false",
        "esphome": latest,
        "old_esphome": pin,
        "project_version": nxt,
        "old_project_version": project,
        "branch": f"deps/esphome-{latest}",
    }


def apply_plan(root: Path, info: dict[str, str], day: str) -> None:
    if info["skip"] == "true":
        raise SystemExit("nothing to apply; the ESPHome pin is already current")
    old_pin = info["old_esphome"]
    new_pin = info["esphome"]
    old_project = info["old_project_version"]
    new_project = info["project_version"]
    targets = pin_files(root, old_pin)
    if not targets:
        raise SystemExit(f"found no files pinning ESPHome {old_pin}")
    for path in targets:
        text = read_text(path)
        updated, count = replace_pin(text, old_pin, new_pin)
        if count < 1:
            raise SystemExit(f"failed to replace {old_pin} in {path}")
        path.write_text(updated, encoding="utf-8")
    for rel in DEVICE_VERSION_FILES:
        path = root / rel
        path.write_text(replace_project_version(read_text(path), old_project, new_project), encoding="utf-8")
    example = root / EXAMPLE_FILE
    example.write_text(replace_example_ref(read_text(example), old_project, new_project), encoding="utf-8")
    changelog = root / CHANGELOG
    changelog.write_text(
        update_changelog(read_text(changelog), old_project, new_project, new_pin, day),
        encoding="utf-8",
    )
    leftover = pin_files(root, old_pin)
    if leftover:
        names = ", ".join(path.relative_to(root).as_posix() for path in leftover)
        raise SystemExit(f"ESPHome pin {old_pin} still present in {names}")


def _write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def _fixture(root: Path) -> None:
    _write(
        root / PUBLISH_WORKFLOW,
        '        uses: esphome/build-action@ae5408cdbc4f70692bf8e345c4982bca05a93667 # v8.1.0\n'
        "        with:\n"
        '          version: "2026.10.1"\n'
        "      - run: pip install esphome==2026.10.1\n",
    )
    _write(
        root / ".github/workflows/validate.yml",
        "      - run: pip install esphome==2026.10.1\n      - run: pip install esphome==2026.10.1\n",
    )
    _write(
        root / "docs/configuration.md",
        "pip install esphome==2026.10.1\nGitHub Actions builds the firmware with ESPHome 2026.10.1.\n"
        "Already documented: ESPHome 2026.10.10 stays.\n",
    )
    _write(root / "documentation/configuration.md", "pip install esphome==2026.10.1\n")
    _write(root / "documentation/configuration.pt.md", "compila o firmware com ESPHome 2026.10.1.\n")
    _write(root / "core/web-set.yaml", "# ESPHome 2026.10 only applies /number/<name>/set on POST.\n")
    for rel in DEVICE_VERSION_FILES:
        _write(root / rel, f'  project_version: "0.8.0"\n')
    _write(root / "devices/setup-m5stickc-plus2.yaml", '  project_version: "0.3.0-beta.1"\n')
    _write(root / EXAMPLE_FILE, "    ref: v0.8.0   # a release tag: immutable, matches the component tag\n")
    _write(
        root / CHANGELOG,
        "# Changelog\n\n"
        "## [Unreleased]\n\n"
        "- Room sensor notes that must ship with this release.\n\n"
        "## [0.8.0] - 2026-11-23\n\n"
        "### Added\n"
        "- Previous.\n\n"
        "[Unreleased]: https://github.com/skydoy/ESPHome_AC/compare/v0.8.0...HEAD\n"
        "[0.8.0]: https://github.com/skydoy/ESPHome_AC/compare/v0.7.0...v0.8.0\n",
    )
    _write(root / "README.md", "No pin here.\n")
    _write(root / "tools/update_esphome_pin.py", 'FIXTURE = "esphome==2026.10.1"\n')


def _self_test() -> None:
    import tempfile

    pypi = {
        "releases": {
            "2026.10.1": [{"yanked": False}],
            "2026.11.0b1": [{"yanked": False}],
            "2026.11.0rc1": [{"yanked": False}],
            "2026.9.5": [{"yanked": False}],
            "2026.11.0": [{"yanked": True}],
            "2024.12.4": [{"yanked": False}],
            "2026.8.0.dev1": [{"yanked": False}],
        }
    }
    if latest_from_pypi(pypi) != "2026.10.1":
        raise SystemExit("PyPI parser picked a pre-release, yanked, or older version")
    pypi["releases"]["2026.11.0"] = [{"yanked": False}]
    if latest_from_pypi(pypi) != "2026.11.0":
        raise SystemExit("PyPI parser did not pick the newest stable release")
    try:
        latest_from_pypi({"releases": {"2026.11.0b2": [{"yanked": False}]}})
    except SystemExit:
        pass
    else:
        raise SystemExit("beta-only catalog should fail")
    if latest_from_github({"tag_name": "2026.11.0", "prerelease": False, "draft": False}) != "2026.11.0":
        raise SystemExit("GitHub tag parse failed")
    try:
        latest_from_github({"tag_name": "2026.11.0b1", "prerelease": True, "draft": False})
    except SystemExit:
        pass
    else:
        raise SystemExit("GitHub prerelease should fail")
    if next_project_version("0.8.0", {"v0.8.0"}) != "0.8.1":
        raise SystemExit("patch bump failed")
    if next_project_version("0.8.0", {"v0.8.0", "v0.8.1"}) != "0.8.2":
        raise SystemExit("existing tag was not skipped")

    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        _fixture(root)
        same = build_plan(root, "2026.10.1", {"v0.8.0"})
        if same["skip"] != "true":
            raise SystemExit("equal pin should skip")
        try:
            build_plan(root, "2026.9.0", set())
        except SystemExit:
            pass
        else:
            raise SystemExit("downgrade should fail")
        try:
            build_plan(root, "2026.11.0b1", set())
        except SystemExit:
            pass
        else:
            raise SystemExit("beta latest should fail")
        info = build_plan(root, "2026.11.0", {"v0.8.0", "v0.8.1"})
        if info["skip"] != "false" or info["project_version"] != "0.8.2" or info["branch"] != "deps/esphome-2026.11.0":
            raise SystemExit(f"unexpected plan {info}")
        apply_plan(root, info, "2026-11-24")
        publish = read_text(root / PUBLISH_WORKFLOW)
        if 'version: "2026.11.0"' not in publish or "esphome==2026.11.0" not in publish:
            raise SystemExit("publish.yml pin was not updated")
        if "2026.10.1" in publish:
            raise SystemExit("old pin left in publish.yml")
        docs = read_text(root / "docs/configuration.md")
        if "ESPHome 2026.11.0" not in docs or "esphome==2026.11.0" not in docs:
            raise SystemExit("docs pin was not updated")
        if "2026.10.10" not in docs:
            raise SystemExit("a longer version was corrupted")
        if read_text(root / "core/web-set.yaml") != "# ESPHome 2026.10 only applies /number/<name>/set on POST.\n":
            raise SystemExit("unrelated ESPHome 2026.10 note was changed")
        if read_text(root / "tools/update_esphome_pin.py") != 'FIXTURE = "esphome==2026.10.1"\n':
            raise SystemExit("the updater script must not be rewritten")
        for rel in DEVICE_VERSION_FILES:
            if 'project_version: "0.8.2"' not in read_text(root / rel):
                raise SystemExit(f"{rel} was not bumped")
        if 'project_version: "0.3.0-beta.1"' not in read_text(root / "devices/setup-m5stickc-plus2.yaml"):
            raise SystemExit("setup firmware version must stay untouched")
        if "ref: v0.8.2" not in read_text(root / EXAMPLE_FILE):
            raise SystemExit("example ref was not bumped")
        changelog = read_text(root / CHANGELOG)
        if "## [0.8.2] - 2026-11-24" not in changelog:
            raise SystemExit("changelog heading missing")
        if "- Updated ESPHome to 2026.11.0." not in changelog:
            raise SystemExit("changelog ESPHome line missing")
        if "- Room sensor notes that must ship with this release." not in changelog.split("## [0.8.0]")[0]:
            raise SystemExit("unreleased notes were not moved into the patch section")
        if "[Unreleased]: https://github.com/skydoy/ESPHome_AC/compare/v0.8.2...HEAD" not in changelog:
            raise SystemExit("unreleased compare link was not moved")
        if "[0.8.2]: https://github.com/skydoy/ESPHome_AC/compare/v0.8.0...v0.8.2" not in changelog:
            raise SystemExit("version compare link missing")
        if "[0.8.0]: https://github.com/skydoy/ESPHome_AC/compare/v0.7.0...v0.8.0" not in changelog:
            raise SystemExit("previous compare link was damaged")
        # Real changelog shape: empty Unreleased, and the function must not write the repo.
        real = Path(CHANGELOG)
        if real.is_file():
            original = real.read_text(encoding="utf-8")
            updated = update_changelog(original, "0.8.0", "0.8.1", "2026.11.0", "2026-11-24")
            if real.read_text(encoding="utf-8") != original:
                raise SystemExit("self-test wrote CHANGELOG.md")
            if "## [Unreleased]\n\n## [0.8.1] - 2026-11-24\n\n- Updated ESPHome to 2026.11.0.\n\n## [0.8.0]" not in updated:
                raise SystemExit("real changelog heading was not inserted cleanly")
            if "v0.8.0...v0.8.1" not in updated or "v0.8.1...HEAD" not in updated:
                raise SystemExit("real changelog links were not updated")
    pub = Path(".github/workflows/publish.yml")
    upd = Path(".github/workflows/update-esphome.yml")
    val = Path(".github/workflows/validate.yml")
    if pub.is_file() and upd.is_file():
        factories = re.compile(r"^\s*factory:\s*(\S+)\s*$", re.M)
        pub_f = factories.findall(pub.read_text(encoding="utf-8"))
        upd_f = factories.findall(upd.read_text(encoding="utf-8"))
        if pub_f != upd_f:
            raise SystemExit(f"factory matrix drifted:\n publish={pub_f}\n update={upd_f}")
    if val.is_file() and upd.is_file():
        devices = re.compile(r"^\s+- (devices/\S+\.yaml)\s*$", re.M)
        val_d = devices.findall(val.read_text(encoding="utf-8"))
        upd_d = devices.findall(upd.read_text(encoding="utf-8"))
        if val_d != upd_d:
            raise SystemExit(f"validate device list drifted:\n validate={val_d}\n update={upd_d}")
    print("update_esphome_pin self-test passed")


def _print_plan(info: dict[str, str]) -> None:
    print(plan_lines(info["skip"] == "true", info["esphome"], info["old_esphome"], info["project_version"], info["old_project_version"]))


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--latest-only", action="store_true")
    parser.add_argument("--plan", action="store_true")
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--latest", default="")
    parser.add_argument("--tags", default="")
    parser.add_argument("--tags-file", default="")
    parser.add_argument("--date", default="")
    parser.add_argument("--root", default=".")
    args = parser.parse_args(argv)
    if args.self_test:
        _self_test()
        return 0
    if args.latest_only:
        print(fetch_latest())
        return 0
    if not args.plan and not args.apply:
        parser.error("choose --latest-only, --plan, --apply, or --self-test")
    if not args.latest:
        parser.error("--latest is required")
    raw_tags = args.tags
    if args.tags_file:
        raw_tags += "\n" + Path(args.tags_file).read_text(encoding="utf-8")
    root = Path(args.root)
    info = build_plan(root, args.latest, parse_tags(raw_tags))
    _print_plan(info)
    if args.apply:
        day = args.date or datetime.now(timezone.utc).strftime("%Y-%m-%d")
        if not re.fullmatch(r"\d{4}-\d{2}-\d{2}", day):
            raise SystemExit(f"bad date {day!r}")
        apply_plan(root, info, day)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
