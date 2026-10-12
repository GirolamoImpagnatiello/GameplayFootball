Canonical VLM configuration selected after the clip-1 comparison tests.

The prompt uses only the standard Cosmos structured-caption JSON schema.
Football-event guidance and the 24 SoccerReplay-1988 labels are instructions
outside the JSON schema. No custom JSON keys are added.

Effective generation settings:
- enable_thinking: true
- reasoning_effort: medium
- preserve_thinking: false
- max_tokens: 32768
- temperature: 0.1
- top_p: 0.9
- top_k: 20
- min_p: 0.0
- video sampling: 8 fps

caption_from_video.py contains the request-level settings and overrides the
server defaults. run_queue.sh processes clip_1 through clip_4 sequentially.
