# SoccerReplay annotation audit

## Scope and result

The MatchVision-facing export retains the 24 canonical SoccerReplay labels. Internal
actions such as passes and tackles are not mapped to an unrelated canonical label.

The referee now records `corner`, `throw in`, `free kick`, and `penalty` when the
set-piece taker has touched the ball, rather than when the restart is awarded.
The cause remains at its own timestamp: `ball out of play`, `lead to corner`,
`off-side`, or the applicable foul/card label. A carded foul no longer also
emits `foul (no card)`.

The repeatable timestamp check is:

```powershell
./tools/audit_soccereplay_timestamps.ps1 -Path out/build-vs2026-x86/output/datasets/soccerreplay1988
```

It checks event IDs, real timestamps, mutually exclusive foul labels at one
instant, and cause/restart labels at one instant. Existing historical exports
must be regenerated to receive the corrected timestamps.

## Remaining semantic checks before training

- `shot off target` is recognized when the goal kick is awarded, but now
  carries the saved timestamp of the preceding shot touch. Check recognition
  errors and video alignment before using these clips. An observed false label
  in historical match `match_20260920_223403` paired a goalkeeper save at
  65.78 s with `shot off target` at 66.99 s; the exporter now suppresses
  `shot off target` after a registered save.
- A card label is recorded when the referee processes the foul. The player's
  card count changes immediately; the delayed `cardEffectiveTime_ms` only
  postpones sending off a player with more than one card. A sampled RGB video
  around the historical yellow-card annotation at 176.82 s shows a stoppage
  but no visible card presentation. Treat card clips as visually unverified
  for video-only classification; the simulator has no explicit card-display
  action in the referee or official-player code.
- `penalty missed` has no emission path. Penalty outcomes need an explicit,
  mutually exclusive mapping before claiming coverage of that class.
- `saved by goal-keeper` requires a recent pending shot. RGB frames at 65.78 s
  and 188.78 s in historical match `match_20260920_223403` visibly show a
  goalkeeper intervention near goal. These two examples support the label,
  but do not establish precision or recall over all saves. The six-second
  pending-shot rule and unlabelled keeper touches still require a larger
  review with per-shot identity.
- The code has no emission paths for `injury` or `var`. Their absence should be
  reported as class coverage, not interpreted as zero incidents in real soccer.
- `ball possession` is a filtered change of team possession, not an annotation
  for every individual touch or pass.

## Evidence and training decision

Five historical complete matches contain 307 exported events, including 13
`saved by goal-keeper`, one `yellow card`, and no `penalty` or `penalty missed`.
These matches predate the timestamp corrections, so they are diagnostic only.

| Case | What is verified | Dataset decision |
| --- | --- | --- |
| Card | The card count increments at the foul; the delay applies to sending off. The sampled yellow-card video has no visible card gesture. | Exclude card clips from video-only supervision until visual evidence is implemented and checked. |
| Penalty missed | The label is canonical but has no emission call; the five complete historical matches contain no penalty event with which to test an outcome. | Report zero coverage; do not synthesize the label from unrelated shots. |
| Goalkeeper save | Two inspected video timestamps show an intervention; one old event was also mislabeled `shot off target`, which has been fixed. | Keep save clips as candidates, subject to visual sampling and per-shot outcome checks. |
| Injury and VAR | Their names occur only in the canonical-label declarations/registration; no gameplay or export path was found. | Report unavailable classes. |
| Ball possession | Only changes in controlling team are emitted, with a five-second filter. | Treat as a coarse state annotation, not a count of touches or passes. |

This audit does not establish precision or recall of the rare labels. A larger
video review requires a shot ID, outcome state, and explicit visual-evidence
status for each generated clip.

Do not use counts from old and regenerated exports as if they followed the same
annotation rules. Record the code revision and configuration with each batch.
