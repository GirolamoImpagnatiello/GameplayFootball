#!/usr/bin/env bash
set -Eeuo pipefail

ROOT=/home/tesista/girolamo_test/test_30s/reasoning_test

exec docker run --rm \
  --name vllm-qwen3-8-flash-next-test30s-reasoning \
  --gpus all \
  --ipc=host \
  --ulimit memlock=-1 \
  --ulimit stack=67108864 \
  -p 8000:8000 \
  -v /home/tesista/.cache/qwen3_8-flash-next-vllm:/root/.cache \
  -v "$ROOT:$ROOT:ro" \
  -e HF_TOKEN="${HF_TOKEN:-}" \
  -e VLLM_PLE_NVFP4=1 \
  -e VLLM_PLE_NVFP4_MMAP=1 \
  -e VLLM_PLE_NVFP4_MMAP_PREWARM=1 \
  --entrypoint vllm \
  jstarkg/vllm-gb10-flashnext:0.28-sm121-r6 \
  serve /root/.cache/models/qwen3.8-flash-next-nvfp4 \
  --served-model-name qwen3.8-flash-next \
  --quantization modelopt_fp4 \
  --moe-backend marlin \
  --gpu-memory-utilization 0.78 \
  --max-model-len 131072 \
  --max-num-batched-tokens 8192 \
  --max-num-seqs 16 \
  --limit-mm-per-prompt '{"image":0,"video":1}' \
  --media-io-kwargs '{"video":{"num_frames":-1,"fps":8}}' \
  --default-chat-template-kwargs '{"enable_thinking":true,"reasoning_effort":"medium","preserve_thinking":false}' \
  --load-format runai_streamer \
  --model-loader-extra-config '{"concurrency":16,"memory_limit":4294967296}' \
  --enable-prefix-caching \
  --mamba-cache-mode align \
  --enforce-eager \
  --speculative-config '{"method":"qwen3_8_flash_next_mtp","num_speculative_tokens":1}' \
  --allowed-local-media-path "$ROOT"
