#!/usr/bin/env python3
"""Check internal links in a built MkDocs site. External URLs are reported, not fetched."""

import sys
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urldefrag, urlparse


class LinkParser(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.refs = []

    def handle_starttag(self, tag, attrs):
        attr = dict(attrs)
        if tag == "a" and attr.get("href"):
            self.refs.append(attr["href"])
        elif tag in {"img", "script"} and attr.get("src"):
            self.refs.append(attr["src"])
        elif tag == "link" and attr.get("href"):
            self.refs.append(attr["href"])


def is_external(href: str) -> bool:
    parsed = urlparse(href)
    return parsed.scheme in {"http", "https", "mailto"} or href.startswith("//")


def resolve(page: Path, root: Path, href: str) -> tuple[Path | None, str]:
    path, frag = urldefrag(href)
    if not path or path.startswith("#"):
        return page, frag
    parsed = urlparse(path)
    if parsed.scheme or parsed.netloc:
        return None, frag
    target = unquote(parsed.path)
    if target.startswith("/"):
        # Site-absolute. MkDocs site_url paths include /ESPHome_AC/docs/.
        marker = "/docs/"
        if marker in target:
            target = target.split(marker, 1)[1]
        else:
            target = target.lstrip("/")
        dest = (root / target).resolve()
    else:
        dest = (page.parent / target).resolve()
    return dest, frag


def page_file(dest: Path) -> Path | None:
    if dest.is_dir():
        index = dest / "index.html"
        return index if index.is_file() else None
    if dest.is_file():
        return dest
    # directory URL without trailing slash
    index = dest / "index.html"
    if index.is_file():
        return index
    html = dest.with_suffix(".html") if dest.suffix == "" else None
    if html and html.is_file():
        return html
    return None


def main() -> int:
    root = Path(sys.argv[1]).resolve()
    if not root.is_dir():
        print(f"missing site dir: {root}", file=sys.stderr)
        return 2
    missing = []
    external = set()
    pages = sorted(root.rglob("*.html"))
    for page in pages:
        parser = LinkParser()
        parser.feed(page.read_text(encoding="utf-8", errors="replace"))
        for href in parser.refs:
            if href.startswith(("javascript:", "data:")):
                continue
            if is_external(href):
                external.add(href.split("#", 1)[0])
                continue
            dest, frag = resolve(page, root, href)
            if dest is None:
                continue
            found = page_file(dest)
            rel = page.relative_to(root)
            if found is None:
                missing.append(f"{rel}: {href} -> {dest}")
                continue
            if frag and found.suffix == ".html":
                text = found.read_text(encoding="utf-8", errors="replace")
                if f'id="{frag}"' not in text and f"id='{frag}'" not in text:
                    missing.append(f"{rel}: missing anchor #{frag} in {found.relative_to(root)} ({href})")
    search_file = root / "search" / "search_index.json"
    pt_page = root / "pt" / "index.html"
    pt_points_at_search = False
    if pt_page.is_file() and search_file.is_file():
        # Material sets "base": ".." on /pt/ so the worker loads <root>/search/search_index.json.
        pt_points_at_search = '"base": ".."' in pt_page.read_text(encoding="utf-8")
    search_ok = search_file.is_file() and pt_points_at_search
    print(f"html pages: {len(pages)}")
    print(f"external urls (not fetched): {len(external)}")
    print(f"search index: {'ok' if search_ok else 'MISSING'}")
    if not search_ok:
        missing.append("search index missing, or the Portuguese pages do not point at it")
    if missing:
        print(f"INTERNAL LINK FAILURES: {len(missing)}")
        for item in missing:
            print(item)
        return 1
    print("internal links: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
