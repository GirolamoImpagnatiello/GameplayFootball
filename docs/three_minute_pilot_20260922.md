# Pilot: ten matches configured for three minutes

Source: `out/build-vs2026-x86/output/datasets/soccerreplay1988_3min_pilot`.
All ten matches have two half-end events and a final summary. The timestamp
auditor reports zero cause/restart conflicts. No video frames or clips were
captured in this pilot, so every clip count below is a timestamp-based
candidate, not a visually validated training example.

## Method

- Raw events: every `frame_index.json` event.
- Potentially useful events: exclude `ball possession`, half starts/ends, and
  `statistics and summary`. This deliberately broad filter still includes
  visually unverified cards and substitutions.
- Valid 30-second window: event center is at least 15 seconds from both the
  start and end of its own half. No padding or crossing the half boundary.
- Nonoverlap clips: maximum-count greedy selection of valid, equal-length
  windows per half. This maximizes temporal count, not class balance.
- Temporal efficiency: union of valid candidate windows divided by
  `30 seconds x valid candidate count`.

## Per-match results

| Match suffix | Actual duration, s | Raw | Useful | Valid windows | Nonoverlap clips |
| --- | ---: | ---: | ---: | ---: | ---: |
| 121905 | 239.14 | 56 | 19 | 14 | 5 |
| 122309 | 202.87 | 46 | 9 | 4 | 2 |
| 122637 | 207.88 | 45 | 11 | 6 | 3 |
| 123010 | 210.73 | 48 | 14 | 11 | 3 |
| 123346 | 226.83 | 54 | 22 | 18 | 6 |
| 123738 | 214.93 | 43 | 8 | 6 | 3 |
| 124118 | 228.70 | 49 | 18 | 14 | 5 |
| 124512 | 227.69 | 48 | 16 | 13 | 4 |
| 124905 | 217.78 | 48 | 14 | 8 | 2 |
| 125248 | 250.22 | 58 | 21 | 15 | 5 |
| **Total** | **2226.77** | **495** | **152** | **109** | **38** |
| **Mean** | **222.68** | **49.5** | **15.2** | **10.9** | **3.8** |

The configured regulation duration was three minutes; the span of
`actual_time_ms` averaged 3.71 minutes because stoppages add time. This is
potential video duration if the match were captured, not a recorded file.
The standard deviation of nonoverlap clips is 1.40 per match. The provisional
95% t interval for the mean is approximately 2.8 to 4.8 clips per match;
this does not include visual-validity or semantic-diversity uncertainty.

## Class distribution

`ball possession` accounts for 293/495 raw annotations (59.2%). Of the 152
potentially useful events, the following were observed:

| Label | Events | Matches | Valid windows |
| --- | ---: | ---: | ---: |
| free kick | 40 | 10 | 33 |
| off-side | 37 | 9 | 32 |
| clearance | 24 | 9 | 19 |
| substitution | 20 | 10 | 0 |
| saved by goal-keeper | 13 | 6 | 10 |
| goal | 6 | 4 | 4 |
| ball out of play | 4 | 4 | 3 |
| yellow card | 4 | 4 | 4 |
| corner | 1 | 1 | 1 |
| penalty | 1 | 1 | 1 |
| shot off target | 1 | 1 | 1 |
| throw in | 1 | 1 | 1 |

Eight canonical labels did not occur: `injury`, `own goal`, `penalty missed`,
`red card`, `second yellow card`, `lead to corner`, `var`, and `foul (no card)`.
The 37 off-sides each have a `free kick` 4.55 to 4.63 seconds later. They are
separate annotations of the same restart sequence and ordinarily share a
30-second clip. The single penalty is followed by `saved by goal-keeper`
0.26 seconds later. These correlations must not be counted as independent
video examples.

## Temporal redundancy and use

The 109 valid event-centered windows nominally total 3270 seconds; their
union covers 1441.95 seconds. Temporal efficiency is therefore 44.1%, or
55.9% duplicated window time. The union covers 64.8% of the 2226.77 seconds
of elapsed match time. Thirty-eight nonoverlapping windows cover 1140 seconds.
The simple earliest-window selection covers nine of the eleven labels with a
valid window; it misses the only `shot off target` and `throw in`. A later
selector should prioritize rare classes and may yield fewer than 38 clips.

Useful-event yield is 4.10 per actual minute. Valid-window yield is 2.94 per
actual minute, and the nonoverlap bound is 1.02 per actual minute. At the
observed mean, 1000 nonoverlapping candidates would require about 264 matches
and 16.3 hours of potential raw video if captured. This is only a pilot extrapolation:
visual validity, class balance, and semantic diversity have not been measured.

Run `tools/analyze_soccereplay_pilot.ps1 -DatasetRoot <directory>` to
reproduce per-match and per-class timestamp statistics.
