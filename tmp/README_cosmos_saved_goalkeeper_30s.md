# Cosmos Transfer — saved by goal-keeper, 30 s

Status: configured and validated locally; inference has not been started.

## Effective settings

- Source action: 30-second clip without the opening celebration.
- Output/input raster: 832x480 (`resolution: 480`).
- Frame rate: 10 fps constant.
- Reference files: 300 frames, exactly 30.0 s.
- Native Cosmos condition files: 301 frames, 30.1 s including the required final endpoint frame cloned from frame 299.
- Steps: 10.
- Seed: 2026.
- Guidance: 4.0.
- Control guidance: 1.0.
- Shift: 7.0.
- Edge threshold preset: `low`.
- Chunk size: 93 frames, inherited from the locally validated 30-second Cosmos benchmark.
- Controls: edge, depth, segmentation.

The positive caption is the final supervised VLM caption in which the central event is conditioned as `saved by goal-keeper`. Only its media metadata was updated to 832x480, 10 fps and 30 s. The negative prompt is copied unchanged from matrix `test_3`.

All four streams use the same temporal edit: source frames 0–35 are replaced by a hold on source frame 36, removing the celebration while preserving the goalkeeper event at 00:15. They are then sampled to 10 fps and resized to 832x480. Edge and segmentation use nearest-neighbor resizing; depth uses bilinear resizing. Control videos use lossless H.264 4:4:4.

Nothing has been uploaded and Cosmos inference has not been launched.
