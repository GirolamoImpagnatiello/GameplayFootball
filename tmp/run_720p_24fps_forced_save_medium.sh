#!/usr/bin/env bash
set -Eeuo pipefail

source /home/tesista/.bash_profile

ROOT=/home/tesista/girolamo_test/test_30s/reasoning_test
TEST="$ROOT/test_720p_24fps_forced_save"
COSMOS_ROOT=/home/tesista/cosmos-framework
PYTHON="$COSMOS_ROOT/.venv/bin/python"
SERVER=http://127.0.0.1:8000/v1
MODEL=qwen3.8-flash-next
LOG="$TEST/queue.log"
STATE="$TEST/state"

mkdir -p "$TEST/logs" "$TEST/output_vlm/output_1"
exec >>"$LOG" 2>&1

mark() {
  printf '%s %s\n' "$(date --iso-8601=seconds)" "$1" | tee "$STATE"
}

cleanup() {
  docker rm -f vllm-qwen3-8-flash-next-test30s-reasoning >/dev/null 2>&1 || true
}
trap cleanup EXIT

mark STARTING_VLLM_SERVER
docker rm -f vllm-qwen3-8-flash-next-test30s-reasoning >/dev/null 2>&1 || true
bash "$ROOT/config/thinking_medium_final/start_vllm.sh" >"$TEST/logs/server.log" 2>&1 &
server_pid=$!

ready=0
for _ in $(seq 1 240); do
  if curl -fsS http://127.0.0.1:8000/health >/dev/null 2>&1 || curl -fsS http://127.0.0.1:8000/v1/health/ready >/dev/null 2>&1; then
    ready=1
    break
  fi
  if ! kill -0 "$server_pid" 2>/dev/null; then
    mark VLLM_SERVER_FAILED
    exit 1
  fi
  sleep 10
done

if [[ "$ready" -ne 1 ]]; then
  mark VLLM_SERVER_TIMEOUT
  exit 1
fi

mark VLLM_SERVER_READY
mark CLIP_1_INFERENCE_STARTED
cd "$COSMOS_ROOT"
"$PYTHON" "$ROOT/config/thinking_medium_final/caption_from_video.py" \
  --video "$TEST/clip_1/input/video/control_rgb.mp4" \
  --output-dir "$TEST/output_vlm/output_1" \
  --server "$SERVER" \
  --model "$MODEL" \
  --max-workers 1 \
  --max-retries 5 \
  --timeout 3600 \
  --prompt-template-path "$TEST/clip_1/input/prompt.txt" \
  --debug \
  >"$TEST/logs/clip_1.log" 2>&1

generated="$TEST/output_vlm/output_1/control_rgb"
test -s "$generated/caption.json"
test -s "$generated/caption.txt"
test -s "$generated/sample_args.json"
shopt -s dotglob nullglob
generated_files=("$generated"/*)
if (( ${#generated_files[@]} > 0 )); then
  mv "${generated_files[@]}" "$TEST/output_vlm/output_1/"
fi
shopt -u dotglob nullglob
rmdir "$generated"
"$PYTHON" -m json.tool "$TEST/output_vlm/output_1/caption.json" >/dev/null
mark CLIP_1_COMPLETE
