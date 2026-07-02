# SoccerReplay-1988 Synthetic Export Contract

This file is the canonical contract for synthetic annotations exported from
GameplayFootball for SoccerReplay-1988 / MatchVision compatibility.

Source specification:
- `C:/Users/Girni/Desktop/Tesi/Documenti/Paper del Prof/2412.01820v3.pdf`
- Dataset format: Appendix A.1
- Event taxonomy and summarization rules: Appendix A.3
- SoccerNet-pro label mapping: Appendix B.1
- MatchVision preprocessing window: Appendix C.1/C.3

## Match Files

Each synthetic match must export two video halves and one JSON annotation file:

```text
match_<id>/
  1-half.mkv
  2-half.mkv
  annotations.json
  frame_index.json
  frames/
  clips/
```

The videos must start at each half kick-off, matching SoccerReplay-1988.
All event `time_stamp` values are relative to the current half, not absolute
match time.

## JSON Top-Level Shape

The annotation JSON must contain four sections:

```json
{
  "match_information": {},
  "referee_information": {},
  "player_information": [],
  "event_descriptions": []
}
```

## Match Information

Use exactly these field names:

```json
{
  "timestamp": "2022-08-07 21:00:00",
  "score": "1 - 2",
  "home_team": "Manchester Utd",
  "away_team": "Brighton",
  "home_formation": "4 - 3 - 3",
  "away_formation": "3 - 4 - 2 - 1",
  "venue": "Old Trafford (Manchester)",
  "capacity": "75 635",
  "attendance": "73 711"
}
```

## Referee Information

Use exactly these field names:

```json
{
  "country": "Eng",
  "name": "Paul Tierney"
}
```

## Player Information

All players, substitutes, absent players, and coaches must be stored in one
unified list. Use exactly these field names, including capitalization:

```json
{
  "players_name": "Caicedo M.",
  "players_number": "25",
  "Full Name": "Moises Caicedo",
  "players_rating": 7.6,
  "Country": "Ecuador",
  "Role": "Midfielder",
  "Age and Birthdate": "22, (02.11.2001)",
  "Market Value": "EUR89.4m"
}
```

## Event Description Format

Each event must use exactly this structure:

```json
{
  "half": 1,
  "time_stamp": "00:16",
  "comments_type": "shot off target",
  "comments_text": "A mistake by Leandro Trossard (Brighton)...",
  "comments_text_anonymized": "A mistake by [PLAYER] ([TEAM])..."
}
```

Rules:
- `half` is `1` or `2`.
- `time_stamp` is `MM:SS` within the half.
- `comments_type` must be one of the 24 canonical labels below, lower-case,
  with the exact spelling shown.
- `comments_text_anonymized` must replace entities with `[PLAYER]`, `[TEAM]`,
  `[COACH]`, and `[REFEREE]`.

## Canonical 24 Event Labels

Use exactly this list for `comments_type`:

```text
corner
goal
injury
own goal
penalty
penalty missed
red card
second yellow card
substitution
start of game(half)
end of game(half)
yellow card
throw in
free kick
saved by goal-keeper
shot off target
clearance
lead to corner
off-side
var
foul (no card)
statistics and summary
ball possession
ball out of play
```

Important compatibility notes:
- The main paper also writes `start of game (half)` and `end of game (half)`
  with a space before the parenthesis. Appendix A.3 uses
  `start of game(half)` and `end of game(half)` inside the prompt. For exact
  prompt compatibility, the exporter must use the no-space form above unless
  a downstream loader explicitly requires title-cased labels.
- Appendix B.1 title-cases processed labels for presentation, e.g.
  `Shot Off Target`, `Throw In`, `Off-Side`. The JSON export should keep the
  lower-case prompt labels above for `comments_type`.
- `statistics and summary` is part of the 24-label taxonomy, but the paper
  excludes these samples from model training and testing because they are not
  visually evidential events.

## Classification Rules From Appendix A.3

These rules must be mirrored when mapping simulator state to labels:

1. If commentary/state evidence indicates a foul, classify as one of:
   `foul (no card)`, `yellow card`, `red card`, `second yellow card`.
   Do not classify the same event as `free kick` or `penalty` just because the
   foul leads to that restart.
2. `lead to corner` describes the process that creates a corner. Use `corner`
   for the actual corner kick.
3. Use `free kick` only for the free-kick attacking action, not for the foul
   that causes it.
4. `penalty` and `penalty missed` describe the penalty kick itself. If the
   event is the cause of a penalty, use the cause label instead.
5. Use `statistics and summary` only for non-visual summaries/statistics and
   exclude it from training/testing exports by default.
6. `ball possession` describes a team controlling possession.
7. A shot that is not a goal and not saved by the goalkeeper should usually be
   `shot off target`.
8. A shot saved or blocked by the goalkeeper is `saved by goal-keeper`.
9. Crosses/passes into the box should not be labeled as `corner` or `free kick`
   unless there is explicit evidence of those events.
10. `clearance` is a successful defensive action that stops an opponent attack.
11. `off-side` must be used for offside events; keep the hyphen.

## MatchVision Alignment

For action recognition with MatchVision:

- Export each annotated event as a 30-frame image clip sampled at 1 FPS.
- The clip window is `[event-15s,event+15s)`, implemented with integer offsets
  `-15, -14, ..., +14`. This gives exactly 30 frames and keeps the action
  timestamp inside the sample.
- Sampling must use rendered video time (`actual_time_ms`), not simulator match
  time (`match_time_ms`). The simulator clock can run faster than real render
  time, so `match_time_ms` is only metadata for SoccerReplay timestamps.
- Near half boundaries, repeat the nearest available rendered frame and mark
  those clip entries with `is_padding: true`.
- Resize/preprocess frames downstream to 224 x 224 with SigLIP-style
  normalization, mean `0.5` and standard deviation `0.5`.
- Keep a sidecar frame index file if needed by Unisoccer, but do not change the
  SoccerReplay-style annotation JSON.

Recommended sidecar:

```json
{
  "event_id": 0,
  "half": 1,
  "comments_type": "shot off target",
  "time_stamp": "00:16",
  "event_match_time_ms": 16000,
  "event_actual_time_ms": 4800,
  "fps": 1,
  "window_duration_seconds": 30,
  "window_start_offset_seconds": -15,
  "window_end_offset_seconds": 14,
  "window_semantics": "[event-15s,event+15s)",
  "clip_directory": "clips/event_000000",
  "clip_frame_count": 30,
  "frame_center": 400,
  "frame_start": 1,
  "frame_end": 19,
  "frame_count": 30,
  "clip_frames": [
    {
      "clip_frame_number": 1,
      "offset_seconds": -15,
      "target_actual_time_ms": -10200,
      "source_frame_number": 1,
      "source_file": "frames/frame_000001.png",
      "clip_file": "clips/event_000000/frame_000001.png",
      "source_actual_time_ms": 0,
      "source_match_time_ms": 0,
      "is_padding": true
    }
  ]
}
```

## Baseline Exporter In This Project

The first implementation is intentionally small and conservative. It exports:

- `start of game(half)`
- `end of game(half)`
- `goal`
- `own goal`
- `ball possession`

The exporter is enabled by default. It can be disabled or redirected in
`football.config`:

```text
dataset_export_enabled=false
dataset_export_root=output/datasets/soccerreplay1988
```

Default output:

```text
output/datasets/soccerreplay1988/match_YYYYMMDD_HHMMSS/
  annotations.json
  frame_index.json
  frames/
    frame_000001.png
    frame_000002.png
  clips/
    event_000000/
      frame_000001.png
      ...
      frame_000030.png
```

`annotations.json` keeps the SoccerReplay-style schema only. Frame metadata is
stored in `frame_index.json` as a sidecar artifact.

Frame dump rules:

- Capture rendered framebuffer PNGs at 1 FPS in video/render time.
- Capture frames before the 2D overlay pass, so dataset images do not include
  scoreboard, radar/minimap, captions, or other HUD elements.
- Keep global frame metadata under `frames`.
- Create one `clips/event_XXXXXX/` directory per annotated event.
- Each event clip contains exactly 30 PNGs sampled at 1 FPS with offsets
  `-15..+14` seconds around the event's `actual_time_ms`.
- Keep the event timestamp inside the selected window by construction.
- Near half boundaries, duplicate the nearest available frame and mark the clip
  entry as padding in `frame_index.json`.

Validation:

```powershell
powershell -ExecutionPolicy Bypass -File tools\validate_synthetic_dataset.ps1 `
  -MatchDirectory out\build-vs2026-x86\output\datasets\soccerreplay1988\match_YYYYMMDD_HHMMSS
```
