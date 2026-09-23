# Comparison of the two ten-match, three-minute pilots

Source: `out/build-vs2026-x86/output/datasets/soccerreplay1988_3min_pilot`.
The first batch contains matches `match_20260922_12*`; the second contains
`match_20260922_13*` and `match_20260922_14*`. Both batches have ten complete
matches. The second batch uses the same nominal three-minute, labels-only
configuration. Random seed is `-1` and teams are randomized, so matches are
not paired. Multiple simulator files changed between runs; the differences
below are descriptive and do not isolate the effect of one code change.

All figures use the definitions in `three_minute_pilot_20260922.md`.
The 30-second windows are timestamp candidates; no video was recorded or
visually checked. The timestamp auditor reports zero issues in the second
batch under its event-pair and timestamp rules.

| Metric | First 10 | Second 10 |
| --- | ---: | ---: |
| Actual elapsed duration, s | 2226.77 | 2301.75 |
| All annotations | 495 | 567 |
| Potentially useful annotations | 152 | 191 |
| Useful annotations per actual minute | 4.10 | 4.98 |
| Valid event-centered 30-second windows | 109 | 135 |
| Maximum nonoverlapping windows | 38 | 34 |
| Nonoverlapping windows per actual minute | 1.02 | 0.89 |
| Temporal efficiency of valid windows | 44.1% | 32.7% |
| Distinct canonical labels observed | 16 | 18 |

More annotations are produced, but they are more concentrated in time. The
135 valid windows in the second batch have 4050 seconds of nominal duration;
their union is 1323.45 seconds. The first batch had 3270 seconds nominal and
1441.95 seconds of union. A class-aware window selector may choose a different
set from the maximum-count selection, especially for rare labels.

| Label | First 10 | Second 10 | Note |
| --- | ---: | ---: | --- |
| ball possession | 293 | 326 | Event count does not measure possession duration or sterile buildup. |
| off-side | 37 | 3 | Large reduction, consistent with the offside-target changes. |
| free kick | 40 | 5 | Previously most were paired with offside. |
| saved by goal-keeper | 13 | 37 | More shot outcomes, but not a direct shot-attempt count. |
| clearance | 24 | 33 | More defensive actions. |
| ball out of play | 4 | 29 | Large increase. |
| corner | 1 | 19 | 18 have a preceding `lead to corner`. |
| lead to corner | 0 | 18 | All 18 precede a corner by 4.40–4.59 s. |
| goal | 6 | 12 | Seven matches contain a goal, versus four previously. |
| penalty | 1 | 3 | Too few to assess a stable rate. |
| shot off target | 1 | 2 | Still sparse. |
| throw in | 1 | 5 | Still sparse. |
| yellow card | 4 | 4 | Unchanged count. |
| foul (no card) | 0 | 1 | Newly observed. |
| substitution | 20 | 20 | Fixed at two per match; no valid window. |

The second batch contains 18 observed canonical labels. Six remain absent:
`injury`, `own goal`, `penalty missed`, `red card`, `second yellow card`, and
`var`. Absence in ten matches does not establish that the simulator cannot
emit any of them.

The second batch is also less uniform across matches: useful annotations
range from 3 to 42, compared with 8 to 22 in the first batch. The standard
deviation is 11.74 versus 4.87. Nonoverlapping candidates range from 1 to 7,
compared with 2 to 6. Ten randomized matches are too few to establish whether
this variability, the doubled goal count, or the modest difference in
nonoverlapping yield will persist. The offside reduction is large, but its
cause should be checked with controlled seeds and otherwise fixed settings.

## Practical interpretation

- The offside issue appears substantially improved in these outputs.
- More goals, saves, and corner sequences improve observed action variety.
- Corner plus lead-to-corner and offside plus free kick describe successive
  stages of one sequence; raw label totals overstate independent examples.
- The present export cannot quantify headed goals, crosses, shot attempts,
  possession duration, or whether possession was sterile. These require
  explicit metadata or a separate event/trajectory analysis.
- For the next experiment, keep each batch in a separate output directory,
  use matched seeds and teams, and compare action sequences and class-balanced
  selected windows as well as raw event totals.

Reproduce summary statistics with
`tools/analyze_soccereplay_pilot.ps1 -DatasetRoot <directory> -MatchNamePattern '^match_20260922_12'`
and then with `-MatchNamePattern '^match_20260922_1[34]'`.
