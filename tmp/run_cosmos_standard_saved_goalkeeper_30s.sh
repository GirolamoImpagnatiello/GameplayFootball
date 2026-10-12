#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="${COSMOS_REPO:-$HOME/cosmos-framework-luca-96303bb}"
PYTHON="${COSMOS_PYTHON:-$HOME/cosmos-framework/.venv/bin/python}"
PATCH_FILE="$ROOT/config/cosmos/luca_multicontrol_terminal_token.patch"
INPUT="$ROOT/config/cosmos/test_saved_goalkeeper_30s.json"
OUTPUT="$ROOT/output_standard"
LOG="$ROOT/run_standard.log"
EXPECTED_COMMIT="96303bb0bdd14d9efa18f24d8ef98c7a0bfb8412"

for required in "$REPO" "$PYTHON" "$PATCH_FILE" "$INPUT"; do
  if [[ ! -e "$required" ]]; then
    echo "Missing required path: $required" >&2
    exit 1
  fi
done

actual_commit="$(git -C "$REPO" rev-parse HEAD)"
if [[ "$actual_commit" != "$EXPECTED_COMMIT" ]]; then
  echo "Unexpected framework commit: $actual_commit" >&2
  exit 1
fi

if ! git -C "$REPO" apply --reverse --check "$PATCH_FILE" >/dev/null 2>&1; then
  echo "Required multi-control terminal-token patch is not applied cleanly." >&2
  exit 1
fi

mkdir -p "$OUTPUT"
cd "$REPO"
export PATH="$HOME/.local/bin:$PATH"
export PYTHONPATH="$REPO"
export LD_LIBRARY_PATH=""
export HF_HOME="${HF_HOME:-$HOME/cosmos-phd/runtime/qwen-hf-home}"
export CUDA_VISIBLE_DEVICES=0
export PYTORCH_CUDA_ALLOC_CONF=expandable_segments:True
export OMP_NUM_THREADS=1

{
  echo "Started: $(date --iso-8601=seconds)"
  echo "Repository: $REPO"
  echo "Commit: $actual_commit"
  echo "Checkpoint: Cosmos3-Nano"
  echo "Input: $INPUT"
  echo "Output: $OUTPUT"
  sha256sum \
    "$ROOT/input/input_rgb_301f.mp4" \
    "$ROOT/input/control_edge_301f.mp4" \
    "$ROOT/input/control_depth_301f.mp4" \
    "$ROOT/input/control_seg_301f.mp4" \
    "$INPUT"

  "$PYTHON" -m cosmos_framework.scripts.inference \
    --parallelism-preset=latency \
    --no-use-torch-compile \
    --model-mode=VIDEO2VIDEO \
    -i "$INPUT" \
    -o "$OUTPUT" \
    --checkpoint-path Cosmos3-Nano \
    --no-guardrails \
    --seed=2026

  echo "Finished: $(date --iso-8601=seconds)"
} 2>&1 | tee "$LOG"
