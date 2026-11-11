#!/usr/bin/env bash
# Copy a finished site-docs/ build to preview-artifact/ESPHome_AC/docs/
# so a local server and the GitHub Actions artifact use the same mount
# as the proposed Pages URL: /ESPHome_AC/docs/
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
src="$root/site-docs"
dest="$root/preview-artifact"
if [[ ! -f "$src/index.html" || ! -f "$src/pt/index.html" ]]; then
  echo "site-docs is missing. Run: mkdocs build --strict" >&2
  exit 1
fi
rm -rf "$dest"
mkdir -p "$dest/ESPHome_AC/docs"
cp -a "$src/." "$dest/ESPHome_AC/docs/"
cat > "$dest/PREVIEW.txt" << 'EOF'
ESPHome_AC documentation preview. Not published.

From this folder (the one that contains ESPHome_AC/ and this file):

  python3 -m http.server 8765

Open:

  http://127.0.0.1:8765/ESPHome_AC/docs/
  http://127.0.0.1:8765/ESPHome_AC/docs/pt/

A local build uses the same layout:

  mkdocs build --strict
  python3 tools/check_docs_links.py site-docs
  bash tools/stage-docs-preview.sh

Language links use /ESPHome_AC/docs/. These URLs are not live on
https://skydoy.github.io/ESPHome_AC/docs/.
EOF
test -f "$dest/ESPHome_AC/docs/index.html"
test -f "$dest/ESPHome_AC/docs/pt/index.html"
echo "staged $dest"
