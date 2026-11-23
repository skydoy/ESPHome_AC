#!/usr/bin/env bash
# Fixture histories for tools/check-commit-authors.sh. No network, no repo writes.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
CHECK="$ROOT/tools/check-commit-authors.sh"
REQ_NAME="skydoy"
REQ_EMAIL="25871317+skydoy@users.noreply.github.com"
AMEND="git commit --amend --author='${REQ_NAME} <${REQ_EMAIL}>'"

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
REPO="$TMP/repo"

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

expect_ok() {
  local name=$1 base=$2 head=$3
  local out status
  set +e
  out=$(bash "$CHECK" "$base" "$head" 2>&1)
  status=$?
  set -e
  if [ "$status" -ne 0 ]; then
    printf '%s\n' "$out" >&2
    fail "$name: exit ${status}, want 0"
  fi
  if grep -F -q -- "$AMEND" <<<"$out"; then
    printf '%s\n' "$out" >&2
    fail "$name: passed but printed the amend instructions"
  fi
  echo "PASS: $name"
}

expect_fail() {
  local name=$1 base=$2 head=$3
  shift 3
  local out status needle
  set +e
  out=$(bash "$CHECK" "$base" "$head" 2>&1)
  status=$?
  set -e
  if [ "$status" -ne 1 ]; then
    printf '%s\n' "$out" >&2
    fail "$name: exit ${status}, want 1"
  fi
  if ! grep -F -q -- "$AMEND" <<<"$out"; then
    printf '%s\n' "$out" >&2
    fail "$name: missing amend command"
  fi
  if ! grep -F -q -- "git rebase <base> --exec" <<<"$out"; then
    printf '%s\n' "$out" >&2
    fail "$name: missing rebase command"
  fi
  for needle in "$@"; do
    if ! grep -F -q -- "$needle" <<<"$out"; then
      printf '%s\n' "$out" >&2
      fail "$name: missing [${needle}]"
    fi
  done
  echo "PASS: $name"
}

commit_file() {
  local path=$1 content=$2 msg=$3 an=$4 ae=$5 cn=$6 ce=$7
  printf '%s\n' "$content" >> "$path"
  git add "$path"
  GIT_AUTHOR_NAME="$an" GIT_AUTHOR_EMAIL="$ae" \
    GIT_COMMITTER_NAME="$cn" GIT_COMMITTER_EMAIL="$ce" \
    git commit -q -m "$msg"
}

git init -q -b main "$REPO"
cd "$REPO"
git config user.name "$REQ_NAME"
git config user.email "$REQ_EMAIL"
git config commit.gpgsign false

# The base commit is outside base..head. A wrong author here must not fail the check.
commit_file readme.txt base "base outside the range" "Other" "other@example.com" "Other" "other@example.com"
BASE=$(git rev-parse HEAD)

git checkout -q -b good
commit_file readme.txt good "good change" "$REQ_NAME" "$REQ_EMAIL" "$REQ_NAME" "$REQ_EMAIL"
GOOD=$(git rev-parse HEAD)

git checkout -q -b bad-email "$BASE"
commit_file readme.txt bad-email "wrong email" "$REQ_NAME" "someone@example.com" "$REQ_NAME" "someone@example.com"
BAD_EMAIL=$(git rev-parse HEAD)

git checkout -q -b bad-name "$BASE"
commit_file readme.txt bad-name "wrong name" "Skydoy" "$REQ_EMAIL" "Skydoy" "$REQ_EMAIL"
BAD_NAME=$(git rev-parse HEAD)

git checkout -q -b bad-middle "$BASE"
commit_file readme.txt ok1 "first ok" "$REQ_NAME" "$REQ_EMAIL" "$REQ_NAME" "$REQ_EMAIL"
commit_file readme.txt bad "middle bad" "skydoy" "not-the-address@example.com" "skydoy" "not-the-address@example.com"
MIDDLE=$(git rev-parse HEAD)
commit_file readme.txt ok2 "last ok" "$REQ_NAME" "$REQ_EMAIL" "$REQ_NAME" "$REQ_EMAIL"
BAD_MIDDLE=$(git rev-parse HEAD)

git checkout -q -b lookalike "$BASE"
commit_file readme.txt lookalike "lookalike email" "$REQ_NAME" "x${REQ_EMAIL}" "$REQ_NAME" "x${REQ_EMAIL}"
LOOKALIKE=$(git rev-parse HEAD)

git checkout -q -b two-bad "$BASE"
commit_file readme.txt one "first bad" "A" "a@example.com" "A" "a@example.com"
FIRST_BAD=$(git rev-parse HEAD)
commit_file readme.txt two "second bad" "B" "b@example.com" "B" "b@example.com"
SECOND_BAD=$(git rev-parse HEAD)

git checkout -q -b injected "$BASE"
commit_file readme.txt injected "injected name" $'bad%0A::error::injected' "evil@example.com" "$REQ_NAME" "$REQ_EMAIL"
INJECTED=$(git rev-parse HEAD)

# Feature tip used as the second parent of the merge fixtures.
git checkout -q -b feature "$BASE"
commit_file feature.txt feature "feature work" "$REQ_NAME" "$REQ_EMAIL" "$REQ_NAME" "$REQ_EMAIL"

merge_ui() {
  local branch=$1 an=$2 ae=$3 cn=$4 ce=$5 message=$6
  git checkout -q -b "$branch" "$BASE"
  commit_file "${branch}.txt" "$branch" "advance ${branch}" "$REQ_NAME" "$REQ_EMAIL" "$REQ_NAME" "$REQ_EMAIL"
  GIT_AUTHOR_NAME="$an" GIT_AUTHOR_EMAIL="$ae" \
    GIT_COMMITTER_NAME="$cn" GIT_COMMITTER_EMAIL="$ce" \
    git merge --no-ff feature -q -m "$message"
  git rev-parse HEAD
}

UI_OK=$(merge_ui ui-ok "Skydoy" "$REQ_EMAIL" "GitHub" "noreply@github.com" "Merge pull request #4 from skydoy/feature")
UI_OTHER_NAME=$(merge_ui ui-other-name "Other Name" "$REQ_EMAIL" "GitHub" "noreply@github.com" "Merge pull request #8 from skydoy/feature")
UI_EXACT=$(merge_ui ui-exact "$REQ_NAME" "$REQ_EMAIL" "GitHub" "noreply@github.com" "Merge pull request #9 from skydoy/feature")
UI_BAD_EMAIL=$(merge_ui ui-bad-email "Skydoy" "febratig10@sapo.pt" "GitHub" "noreply@github.com" "Merge pull request #5 from skydoy/feature")
LOCAL_BAD_NAME=$(merge_ui local-bad-name "Skydoy" "$REQ_EMAIL" "Skydoy" "$REQ_EMAIL" "Merge branch 'feature'")
LOCAL_OK=$(merge_ui local-ok "$REQ_NAME" "$REQ_EMAIL" "$REQ_NAME" "$REQ_EMAIL" "Merge branch 'feature'")
COMMITTER_CASE=$(merge_ui committer-case "Skydoy" "$REQ_EMAIL" "github" "noreply@github.com" "Merge branch 'feature'")
COMMITTER_EMAIL=$(merge_ui committer-email "Skydoy" "$REQ_EMAIL" "GitHub" "24871317+skydoy@users.noreply.github.com" "Merge branch 'feature'")

git checkout -q -b squash "$BASE"
commit_file squash.txt squash "squash style" "Skydoy" "$REQ_EMAIL" "GitHub" "noreply@github.com"
SQUASH=$(git rev-parse HEAD)

expect_ok "required author passes" "$BASE" "$GOOD"
expect_ok "base commit outside the range is not checked" "$BASE" "$GOOD"
expect_ok "empty range passes" "$BASE" "$BASE"
expect_ok "GitHub UI merge with the noreply author email passes (display name Skydoy)" "$BASE" "$UI_OK"
expect_ok "GitHub UI merge with the noreply author email passes (any author name)" "$BASE" "$UI_OTHER_NAME"
expect_ok "GitHub UI merge with the exact required author passes" "$BASE" "$UI_EXACT"
expect_ok "local merge with the required author passes" "$BASE" "$LOCAL_OK"

expect_fail "wrong email fails" "$BASE" "$BAD_EMAIL" "$BAD_EMAIL" "someone@example.com"
expect_fail "wrong name fails" "$BASE" "$BAD_NAME" "$BAD_NAME" "Skydoy"
expect_fail "bad commit in the middle fails" "$BASE" "$BAD_MIDDLE" "$MIDDLE" "not-the-address@example.com"
expect_fail "lookalike email fails" "$BASE" "$LOOKALIKE" "x${REQ_EMAIL}"
expect_fail "every bad commit is reported" "$BASE" "$SECOND_BAD" "$FIRST_BAD" "$SECOND_BAD"
expect_fail "author name cannot inject a workflow command" "$BASE" "$INJECTED" "%250A"
expect_fail "GitHub UI merge with a different author email fails" "$BASE" "$UI_BAD_EMAIL" "febratig10@sapo.pt" "GitHub UI merge"
expect_fail "local merge is not exempt when the author name differs" "$BASE" "$LOCAL_BAD_NAME" "Skydoy"
expect_fail "committer name must be exactly GitHub" "$BASE" "$COMMITTER_CASE" "Skydoy"
expect_fail "committer email must be noreply@github.com" "$BASE" "$COMMITTER_EMAIL" "Skydoy"
expect_fail "single-parent commit committed by GitHub is not a UI merge" "$BASE" "$SQUASH" "Skydoy"

set +e
out=$(bash "$CHECK" 2>&1)
status=$?
set -e
if [ "$status" -ne 2 ]; then
  printf '%s\n' "$out" >&2
  fail "usage: exit ${status}, want 2"
fi
echo "PASS: missing arguments exit 2"

set +e
out=$(bash "$CHECK" "deadbeefdeadbeefdeadbeefdeadbeefdeadbeef" "$GOOD" 2>&1)
status=$?
set -e
if [ "$status" -ne 2 ]; then
  printf '%s\n' "$out" >&2
  fail "missing base: exit ${status}, want 2"
fi
if ! grep -F -q -- "Base commit not found" <<<"$out"; then
  printf '%s\n' "$out" >&2
  fail "missing base: error does not name the base commit"
fi
echo "PASS: missing base commit exits 2"

echo "All author-check fixtures passed."
