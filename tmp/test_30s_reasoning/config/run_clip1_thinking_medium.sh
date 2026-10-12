#!/usr/bin/env bash
set -Eeuo pipefail

source /home/tesista/.bash_profile

ROOT=/home/tesista/girolamo_test/test_30s/reasoning_test
COSMOS_ROOT=/home/tesista/cosmos-framework
PYTHON="$COSMOS_ROOT/.venv/bin/python"
SERVER=http://127.0.0.1:8000/v1
MODEL=qwen3.8-flash-next
LOG="$ROOT/queue.log"
STATE="$ROOT/state"

mkdir -p "$ROOT/logs" "$ROOT/output_vlm"
exec >>"$LOG" 2>&1

mark() {
  printf '%s %s\n' "$(date --iso-8601=seconds)" "$1" | tee "$STATE"
}

cleanup() {
  docker rm -f vllm-qwen3-8-flash-next-test30s-reasoning >/dev/null 2>&1 || true
}

trap cleanup EXIT

mark CLIP1_MEDIUM_STARTING_VLLM_SERVER
docker rm -f vllm-qwen3-8-flash-next-test30s-reasoning >/dev/null 2>&1 || true
"$ROOT/config/start_vllm.sh" >"$ROOT/logs/server.log" 2>&1 &
server_pid=$!

ready=0
for _ in $(seq 1 240); do
  if curl -fsS http://127.0.0.1:8000/health >/dev/null 2>&1 || curl -fsS http://127.0.0.1:8000/v1/health/ready >/dev/null 2>&1; then
    ready=1
    break
  fi
  if ! kill -0 "$server_pid" 2>/dev/null; then
    mark CLIP1_MEDIUM_VLLM_SERVER_FAILED
    exit 1
  fi
  sleep 10
done

if [[ "$ready" -ne 1 ]]; then
    mark CLIP1_MEDIUM_VLLM_SERVER_TIMEOUT
  exit 1
fi

mark CLIP1_MEDIUM_VLLM_SERVER_READY

for index in 1; do
  video="$ROOT/clip_$index/input/video/control_rgb.mp4"
  prompt="$ROOT/clip_$index/input/prompt.txt"
  output="$ROOT/output_vlm/output_$index"

  test -f "$video"
  test -f "$prompt"
  mkdir -p "$output"

  if [[ -s "$output/caption.json" && -s "$output/caption.txt" && -s "$output/sample_args.json" ]]; then
    "$PYTHON" -m json.tool "$output/caption.json" >/dev/null
    mark "CLIP1_MEDIUM_ALREADY_COMPLETE"
    continue
  fi

  mark "CLIP1_MEDIUM_INFERENCE_STARTED"
  cd "$COSMOS_ROOT"
  "$PYTHON" "$ROOT/config/caption_from_video_reasoning.py" \
    --video "$video" \
    --output-dir "$output" \
    --server "$SERVER" \
    --model "$MODEL" \
    --max-workers 1 \
    --max-retries 5 \
    --timeout 3600 \
    --prompt-template-path "$prompt" \
    --debug \
    >"$ROOT/logs/clip1_standard_schema_thinking_medium.log" 2>&1

  generated="$output/control_rgb"
  test -s "$generated/caption.json"
  test -s "$generated/caption.txt"
  test -s "$generated/sample_args.json"

  shopt -s dotglob nullglob
  generated_files=("$generated"/*)
  if (( ${#generated_files[@]} > 0 )); then
    mv "${generated_files[@]}" "$output/"
  fi
  shopt -u dotglob nullglob
  rmdir "$generated"

  "$PYTHON" -m json.tool "$output/caption.json" >/dev/null
  mark "CLIP1_MEDIUM_COMPLETE"
done

mark CLIP1_MEDIUM_ONLY_COMPLETE
