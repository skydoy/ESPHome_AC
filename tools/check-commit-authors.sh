#!/usr/bin/env bash
# Fail when a commit in BASE..HEAD is not authored by the required identity.
#
# Required author: skydoy <25871317+skydoy@users.noreply.github.com>
#
# A merge commit created by the GitHub UI (two or more parents, committer
# GitHub <noreply@github.com>) is ignored only when its author email is that
# noreply address. GitHub records the account display name on those commits,
# so the author name is not checked for that one case. Any other author email
# fails, including on a GitHub UI merge commit.
#
# Usage, from the checkout that contains both commits:
#   tools/check-commit-authors.sh <base-sha> <head-sha>
set -euo pipefail

REQUIRED_NAME="skydoy"
REQUIRED_EMAIL="25871317+skydoy@users.noreply.github.com"
AMEND="git commit --amend --author='${REQUIRED_NAME} <${REQUIRED_EMAIL}>'"
REBASE="git rebase <base> --exec \"git commit --amend --author='${REQUIRED_NAME} <${REQUIRED_EMAIL}>' --no-edit\""

GITHUB_COMMITTER_NAME="GitHub"
GITHUB_COMMITTER_EMAIL="noreply@github.com"

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <base-sha> <head-sha>" >&2
  echo "Checks every commit in base..head (the base commit itself is not included)." >&2
  exit 2
fi

BASE=$1
HEAD=$2

if ! git rev-parse --verify --quiet "${BASE}^{commit}" >/dev/null; then
  echo "::error::Base commit not found: ${BASE}. Fetch the pull request history (actions/checkout fetch-depth: 0)."
  exit 2
fi
if ! git rev-parse --verify --quiet "${HEAD}^{commit}" >/dev/null; then
  echo "::error::Head commit not found: ${HEAD}. Fetch the pull request history (actions/checkout fetch-depth: 0)."
  exit 2
fi

# One physical line, with % escaped so a commit author cannot inject a GitHub
# workflow command (%0A / %0D) through ::error::.
one_line() {
  printf '%s' "$1" | tr '\r\n' '  ' | sed 's/%/%25/g'
}

parent_count() {
  git rev-list --parents -n 1 "$1" | awk '{print NF - 1}'
}

# True when this is a merge commit recorded by the GitHub web UI.
github_ui_merge() {
  local sha=$1 n cname cemail
  n=$(parent_count "$sha")
  [ "$n" -ge 2 ] || return 1
  cname=$(git log -1 --format='%cn' "$sha")
  cemail=$(git log -1 --format='%ce' "$sha")
  [ "$cname" = "$GITHUB_COMMITTER_NAME" ] && [ "$cemail" = "$GITHUB_COMMITTER_EMAIL" ]
}

list=$(git rev-list --reverse "${BASE}..${HEAD}")
failed=0
checked=0
skipped=0

print_fix() {
  cat <<EOF
How to fix: set the author on the commit, then push the branch again.
  ${AMEND}
  ${REBASE}
A merge commit created by the GitHub UI is accepted only when its author email is ${REQUIRED_EMAIL}.
EOF
}

if [ -n "$list" ]; then
  while IFS= read -r sha; do
    [ -n "$sha" ] || continue
    checked=$((checked + 1))
    name=$(git log -1 --format='%an' "$sha")
    email=$(git log -1 --format='%ae' "$sha")
    subject=$(git log -1 --format='%s' "$sha")
    safe_name=$(one_line "$name")
    safe_email=$(one_line "$email")
    safe_subject=$(one_line "$subject")

    if github_ui_merge "$sha" && [ "$email" = "$REQUIRED_EMAIL" ]; then
      skipped=$((skipped + 1))
      echo "ok: ${sha} GitHub UI merge commit, author email is ${REQUIRED_EMAIL} (author name '${safe_name}')"
      continue
    fi

    if [ "$email" = "$REQUIRED_EMAIL" ] && [ "$name" = "$REQUIRED_NAME" ]; then
      echo "ok: ${sha} ${REQUIRED_NAME} <${REQUIRED_EMAIL}> ${safe_subject}"
      continue
    fi

    failed=1
    if github_ui_merge "$sha"; then
      echo "::error::Commit ${sha} is a GitHub UI merge commit but its author is '${safe_name} <${safe_email}>', not '${REQUIRED_NAME} <${REQUIRED_EMAIL}>'. It is ignored only when the author email is ${REQUIRED_EMAIL}. Subject: ${safe_subject}"
    else
      echo "::error::Commit ${sha} author is '${safe_name} <${safe_email}>'; expected '${REQUIRED_NAME} <${REQUIRED_EMAIL}>'. Subject: ${safe_subject}"
    fi
  done <<< "$list"
fi

if [ "$failed" -ne 0 ]; then
  print_fix
  echo "Checked ${checked} commit(s) in ${BASE}..${HEAD}; author check failed."
  exit 1
fi

if [ "$checked" -eq 0 ]; then
  echo "No commits in ${BASE}..${HEAD}. Author check passed."
  exit 0
fi

echo "Checked ${checked} commit(s) in ${BASE}..${HEAD} (${skipped} GitHub UI merge commit(s) with author email ${REQUIRED_EMAIL}). Author check passed."
exit 0
