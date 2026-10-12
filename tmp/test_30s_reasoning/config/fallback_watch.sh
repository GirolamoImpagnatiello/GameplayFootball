#!/usr/bin/env bash
set -Eeuo pipefail

ROOT=/home/tesista/girolamo_test/test_30s/reasoning_test
PRIMARY_SESSION=test30s_reasoning

while tmux has-session -t "$PRIMARY_SESSION" 2>/dev/null; do
  sleep 10
done

if grep -qE ' COMPLETE$' "$ROOT/state" 2>/dev/null; then
  printf '%s PRIMARY_COMPLETE_FALLBACK_NOT_REQUIRED\n' "$(date --iso-8601=seconds)" >>"$ROOT/fallback.log"
  exit 0
fi

printf '%s PRIMARY_FAILED_STARTING_NO_THINKING_FALLBACK\n' "$(date --iso-8601=seconds)" >>"$ROOT/fallback.log"
exec "$ROOT/config/run_fallback_no_thinking.sh"
