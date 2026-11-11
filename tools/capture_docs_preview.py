#!/usr/bin/env python3
"""Capture real browser screenshots of the local docs preview and exercise the UI."""

import json
import sys
from pathlib import Path

from playwright.sync_api import sync_playwright

BASE = sys.argv[1].rstrip("/") + "/"
OUT = Path(sys.argv[2])
OUT.mkdir(parents=True, exist_ok=True)
CHROME = "/usr/bin/google-chrome-stable"
report = {"base": BASE, "checks": [], "shots": []}


def check(name, ok, detail=""):
    report["checks"].append({"name": name, "ok": bool(ok), "detail": detail})
    print(("OK  " if ok else "FAIL") + f" {name}" + (f" — {detail}" if detail else ""))


def shot(page, name):
    path = OUT / name
    page.screenshot(path=str(path), full_page=False)
    report["shots"].append(str(path))
    print(f"SHOT {path}")


def overflow(page):
    return page.evaluate(
        """() => {
          const doc = document.documentElement;
          const wrap = document.querySelector('.md-typeset__scrollwrap');
          return {
            page: doc.scrollWidth - doc.clientWidth,
            client: doc.clientWidth,
            scroll: doc.scrollWidth,
            table: wrap ? {scroll: wrap.scrollWidth, client: wrap.clientWidth} : null
          };
        }"""
    )


def scheme(page):
    return page.evaluate("() => document.body.getAttribute('data-md-color-scheme')")


def set_scheme(page, name):
    page.evaluate(
        """(name) => {
          const input = document.querySelector(`input[name="__palette"][data-md-color-scheme="${name}"]`);
          input.click();
        }""",
        name,
    )
    page.wait_for_timeout(250)


def bg(page):
    return page.evaluate("() => getComputedStyle(document.body).backgroundColor")


def main():
    with sync_playwright() as p:
        browser = p.chromium.launch(
            executable_path=CHROME,
            args=["--no-sandbox", "--disable-dev-shm-usage"],
        )
        desktop = browser.new_context(
            viewport={"width": 1440, "height": 900},
            color_scheme="light",
            device_scale_factor=1,
        )
        page = desktop.new_page()
        page.goto(BASE, wait_until="networkidle")
        check("en home title", "ESPHome_AC" in page.title(), page.title())
        check("en light scheme", scheme(page) == "default", scheme(page))
        check("en light background", bg(page) in {"rgb(244, 248, 252)", "rgb(244, 248, 252)"}, bg(page))
        shot(page, "docs-desktop-en-light-home.png")

        page.locator("a.md-nav__link", has_text="Getting started").first.click()
        page.wait_for_load_state("networkidle")
        check("internal link getting started", page.url.rstrip("/").endswith("/getting-started"), page.url)
        heading = page.locator("h1").inner_text().replace("¶", "").strip()
        check("getting started heading", heading == "Getting started", heading)

        page.goto(BASE, wait_until="networkidle")
        page.locator("a.lang-pill[hreflang='pt']").click()
        page.wait_for_load_state("networkidle")
        check("language switch to pt", "/pt/" in page.url, page.url)
        check("pt pitch", "O teu ar condicionado" in page.locator("article").inner_text())
        check("pt search placeholder", page.locator(".md-search__input").get_attribute("placeholder") == "Pesquisar")
        shot(page, "docs-desktop-pt-light-home.png")

        set_scheme(page, "slate")
        check("pt dark scheme", scheme(page) == "slate", scheme(page))
        check("pt dark background", bg(page) == "rgb(11, 18, 32)", bg(page))
        page.locator("a.md-nav__link", has_text="Ligações").first.click()
        page.wait_for_load_state("networkidle")
        check("pt wiring", page.url.rstrip("/").endswith("/pt/wiring"), page.url)
        shot(page, "docs-desktop-pt-dark-wiring.png")

        page.goto(BASE, wait_until="networkidle")
        set_scheme(page, "slate")
        check("en dark scheme", scheme(page) == "slate", scheme(page))
        shot(page, "docs-desktop-en-dark-home.png")

        page.locator(".md-search__input").click()
        page.locator(".md-search__input").press_sequentially("pull-up", delay=30)
        page.locator(".md-search-result__link").first.wait_for(timeout=8000)
        results = page.locator(".md-search-result__link")
        check("search has results", results.count() > 0, page.locator(".md-search-result__meta").inner_text())
        shot(page, "docs-desktop-en-dark-search.png")
        desktop.close()

        mobile = browser.new_context(
            viewport={"width": 390, "height": 844},
            color_scheme="light",
            device_scale_factor=2,
            is_mobile=True,
            has_touch=True,
        )
        m = mobile.new_page()
        m.goto(BASE, wait_until="networkidle")
        home_overflow = overflow(m)
        check("mobile home no page overflow", home_overflow["page"] <= 1, json.dumps(home_overflow))
        shot(m, "docs-mobile-en-light-home.png")
        m.locator(".md-header__button[for='__drawer']").click()
        m.wait_for_timeout(400)
        check("mobile drawer link", m.locator(".md-nav__link", has_text="Wiring").count() > 0)
        shot(m, "docs-mobile-en-light-nav.png")
        m.locator("a.md-nav__link:visible", has_text="Compatibility").click()
        m.wait_for_load_state("networkidle")
        table = overflow(m)
        check("mobile compatibility no page overflow", table["page"] <= 1, json.dumps(table))
        m.locator("table").first.scroll_into_view_if_needed()
        shot(m, "docs-mobile-en-light-compatibility.png")

        m.locator("a.lang-pill[hreflang='pt']").click()
        m.wait_for_load_state("networkidle")
        check("mobile language pt", "/pt/" in m.url, m.url)
        set_scheme(m, "slate")
        check("mobile pt dark", scheme(m) == "slate", scheme(m))
        m.locator(".md-header__button[for='__drawer']").click()
        m.wait_for_timeout(400)
        shot(m, "docs-mobile-pt-dark-nav.png")
        mobile.close()
        browser.close()

    report_path = OUT / "docs-preview-browser-checks.json"
    report_path.write_text(json.dumps(report, indent=2))
    failed = [c for c in report["checks"] if not c["ok"]]
    print(f"failed {len(failed)} / {len(report['checks'])}")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
