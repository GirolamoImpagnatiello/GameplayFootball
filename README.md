## Gameplay Football
Football game, a fork of discontinued [GameplayFootball](https://github.com/BazkieBumpercar/GameplayFootball) written by [Bastiaan Konings Schuiling](http://www.properlydecent.com/).

In 2019, Google Brain team picked up a game and created a Reinforcement Learning environment based on it - [Google Research Football](https://github.com/google-research/football). They made some improvements to the game, updated the libraries, but threw away everything (e.g. menus, audio effects, etc.) that was not necessary for their task.

The goal of this repository is to update the existing code, based on Google Brain's changes (see `google_brain` branch) and other forks, and make it compiling and running on as many platforms as possible. PRs are always welcome.  

## Building from source

### Linux
Install required dependencies: 
```bash
sudo apt-get install git cmake build-essential libgl1-mesa-dev libsdl2-dev \
libsdl2-image-dev libsdl2-ttf-dev libsdl2-gfx-dev libopenal-dev libboost-all-dev \
libdirectfb-dev libst-dev mesa-utils xvfb x11vnc libsqlite3-dev
```

Run the following commands:
```bash
# Clone the repository
git clone https://github.com/vi3itor/GameplayFootball.git
cd GameplayFootball

# Copy the contents of `data` directory into `build`
cp -R data/. build

# Go to `build` directory
cd build
# Generate Makefile
cmake ..
# Compile the game
make -j$(nproc)
```

Run the game:
```bash
./gameplayfootball
```

### MacOS (Work in Progress)
**Important**: Currently, the game can be compiled on Mac OS, but it is not running yet, because rendering must be done on the Main Thread.

To install required dependencies you need [`brew`](https://brew.sh/) which can be installed in Terminal by running:
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
```

```bash
# Install dependencies
brew install git cmake sdl2 sdl2_image sdl2_ttf sdl2_gfx boost openal-soft
# Navigate to the directory where you want to put the repository
cd ~
# Clone the repository
git clone https://github.com/vi3itor/GameplayFootball.git
cd GameplayFootball
# Copy the contents of `data` directory into `build`
cp -R data/. build

# Go to `build` directory
cd build
# Generate Makefile
cmake ..
# Compile the game
make -j$(nproc)

# Run the game (Currently is not working)
./gameplayfootball
```



### Windows (Work in Progress)

Download and install:
- [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/),
- [Git](https://git-scm.com/download/win),
- [CMake](https://cmake.org/download/) (make sure to add it to the system PATH).

Install [`vcpkg`](https://github.com/microsoft/vcpkg) as explained in [Quick Start Guide](https://github.com/microsoft/vcpkg#quick-start-windows) or simply:
create a directory, e.g. `C:\dev`, open Command Prompt and run the following commands: 
```bat
% Navigate to the created directory
cd C:\dev

% Clone vckpg
git clone https://github.com/microsoft/vcpkg

% Run installation script
.\vcpkg\bootstrap-vcpkg.bat
```
Install required dependencies (all triplets **must be `x86-windows`**):
```bat 
.\vcpkg.exe install --triplet x86-windows boost:x86-windows sdl2 sdl2-image[libjpeg-turbo] sdl2-ttf sdl2-gfx opengl openal-soft
```

```bat
% Navigate to the directory where you want to put the repository
cd C:\dev

% Clone repository
git clone https://github.com/vi3itor/GameplayFootball.git 
cd GameplayFootball

% Switch to windows branch
git switch windows


% Copy the contents of `data` directory into `build\Debug` or (and) `build\Release`
xcopy /e /i data build\Debug
xcopy /e /i data build\Release
```
Go to `build` directory and generate `cmake` files. Make sure that you correctly set the directory for `vcpkg` (in our case it is installed into `C:\dev\vcpkg`):
```bat
cd build

cmake .. -DCMAKE_GENERATOR_PLATFORM=Win32 -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE  
```
To build `Release` version:
```bat
cmake --build . --parallel --config Release
```
For `Debug` version:
```bat
cmake --build . --parallel --config Debug
```

That's it! Run `gameplayfootball.exe` inside `build\Release` directory (or inside `build\Debug` for `Debug` version)

## Cosmos video-to-video capture

The simulator can export a short Cosmos Transfer capture with synchronized RGB, depth and segmentation PNG sequences. Add these optional keys to the config file used at startup:

```txt
"cosmos_capture_enabled" "true"
"cosmos_capture_fps" "30"
"cosmos_capture_frame_count" "121"
"cosmos_capture_skip_frames" "0"
"cosmos_capture_root" "output/cosmos_transfer"
"cosmos_capture_prompt" "A photorealistic broadcast soccer match in a modern stadium, realistic grass, players, ball, lighting and camera motion."
```

### Broadcast camera director

The match camera supports `legacy`, `single`, `scheduled`, and `procedural`
modes. Preset anchors are expressed relative to the 110 x 72 metre pitch, and
camera changes are hard cuts; pan, tilt, and FOV remain smooth within a shot.

```text
"camera_mode" "scheduled"
"single_camera" "false"
"camera_changes" "true"
"camera_schedule" "0:MAIN_BROADCAST,4:HIGH_GOAL_SIDE,7:MAIN_BROADCAST"
"camera_seed" "2026"
"camera_clip_duration_s" "5"
"camera_max_cuts_per_clip" "1"
```

Available presets are `MAIN_BROADCAST`, `HIGH_WIDE`, `HIGH_GOAL_SIDE`,
`LOW_TOUCHLINE`, `LOW_GOAL_SIDE`, `CORNER`, `BEHIND_GOAL`, and the optional
`CLOSE_UP`. Set `camera_mode=procedural` and `single_camera=false` for weighted,
context-aware, seed-reproducible selection. Each weight can be overridden with
`camera_<PRESET>_weight`; shot bounds use `camera_<PRESET>_min_shot_s` and
`camera_<PRESET>_max_shot_s`, and the base FOV uses
`camera_<PRESET>_fov`. `camera_close_up_enabled` defaults to false and close-ups
are eligible only while play is stopped.

Cosmos captures also write `camera_metadata.jsonl`, one record per rendered
frame, with timestamp, preset, physical camera id, position, quaternion, and
FOV. The existing RGB, depth, and segmentation requests still share the active
camera for the frame.

When a match enters normal play, the capture is written under `output/cosmos_transfer/capture_YYYYMMDD_HHMMSS` with `rgb`, `depth`, `seg`, `metadata.json`, `prompt.json` and `cosmos_transfer_spec.json`.

Capture defaults to `"cosmos_capture_lockstep" "true"`: physics and rendering run in order, and slow image writes delay simulation instead of losing frames. At `"cosmos_capture_fps" "25"`, consecutive images represent exactly 40 ms of simulation, even when producing 10 seconds of video takes longer than 10 seconds. Camera and player smoothing also use simulation time. The bounded GPU and PNG queues keep capture memory from growing with recording length.

`metadata.json` includes `lockstep`, `simulation_timestamps_ms` (one simulation timestamp per image triplet), and `dropped_timing_buckets`. The physics step is 10 ms; rates that do not divide 100 have sampling intervals quantized to that step. Set `"cosmos_capture_lockstep" "false"` only to use the legacy independent simulation/rendering sequences, which may skip sampling buckets on slow hardware.

To verify a completed 250-frame capture at 25 fps, including every image and its simulation timestamp:

```powershell
.\tools\validate_cosmos_capture.ps1 -CaptureDirectory output\cosmos_transfer\capture_YYYYMMDD_HHMMSS -ExpectedFrames 250 -ExpectedFps 25 -DecodeImages
```

To build the three MP4 control videos after capture:

```powershell
.\tools\build_cosmos_capture_videos.ps1 -CaptureDirectory output\cosmos_transfer\capture_YYYYMMDD_HHMMSS
```

## Unattended dataset batch

### Direct lossless video capture (Windows)

The tested video profile records three synchronized FFV1/Matroska masters:
`control_rgb.mkv`, `control_depth.mkv`, and `control_seg.mkv`. FFmpeg receives
raw frames through bounded pipes; physics/render timing is unchanged from the
lockstep PNG mode. The videos carry 25 frames per **simulation** second even
when recording runs slower than real time. The depth mapping remains the same
8-bit inverted device depth used by the PNG exporter; it is not metric depth.

```txt
"cosmos_capture_enabled" "true"
"cosmos_capture_format" "video"
"cosmos_capture_lockstep" "true"
"cosmos_capture_fps" "25"
"cosmos_capture_frame_count" "0"
"cosmos_segmentation_team_aware" "true"
"cosmos_ffmpeg_path" "ffmpeg.exe"
"dataset_export_enabled" "true"
"dataset_export_frames" "false"
```

`frame_count=0` records the match's normal halves through game over; a positive
limit produces a short capture. `cosmos_capture_quit_when_complete=true` exits
cleanly after a positive frame limit, for smoke tests. Quit normally to finalize
the encoder streams; forced process termination can leave incomplete captures.
FFmpeg failures are logged and abort the run rather than marking it complete.
FFV1 is lossless but produces large masters: this mode targets capture speed and
label preservation, not minimum disk space.

Team-aware colors are assigned from simulation identity, independent of kits or
field side: home `[0,0,255]`, away `[255,0,0]`, officials `[0,255,255]`.
Goalkeepers and hair use their team's label. Existing field/ball/background
classes remain present. This replaces team classification from RGB pixels;
it does not claim to reproduce an external preprocessing palette. Colors are
recorded in `metadata.json`. `video_and_png` writes both forms for comparisons;
`png` retains the original exporter.

Annotation-only dataset export keeps event timestamps in `frame_index.json`
without creating the legacy 1 fps PNG clips. Its path is stored as `event_index`
in the video metadata. The normal image-dataset validator does not apply to
this annotation-only mode.

Extract aligned PNGs **after** recording (or omit `-Png` for lossless video clips):

```powershell
.\tools\extract_capture_video.ps1 -CaptureDirectory <capture> -OutputDirectory <new-folder> -StartFrame 1 -FrameCount 750 -Png
.\tools\extract_capture_video.ps1 -CaptureDirectory <capture> -OutputDirectory <new-folder> -EventTimeMs 45000 -Png
.\tools\validate_capture_video.ps1 -CaptureDirectory <capture> -ExpectedFrames 750
```

`EventTimeMs` uses `actual_time_ms` from the event index, not the accelerated
scoreboard clock. It selects the first sample at or after `event-15s` and exactly
30 seconds of samples (event alignment is quantized to 40 ms at 25 fps).
Extraction rejects insufficient footage and temporal gaps rather than padding
or duplicating frames. New output folders are required to avoid stale images.

`build_cosmos_capture_videos.ps1` also accepts direct video captures, converting
their MKV masters to the existing H.264/YUV420 MP4 delivery format without PNG
intermediates. That delivery conversion is lossy, just as in the prior PNG-to-MP4
workflow: retain the MKV masters for exact depth/segmentation values. The Cosmos
spec keeps Edge off, guidance 4.0, control guidance 1.0, 35 steps and seed 2026.

The simulator can run a complete AI-versus-AI batch without team-selection,
match-option, half-time, replay, pause, or game-over interaction. Add or update
these keys in the config file passed to `gameplayfootball`:

```txt
"automatic_batch_enabled" "true"
"automatic_match_count" "10"
"automatic_random_teams" "true"
"automatic_home_team_id" "3"
"automatic_away_team_id" "8"
"automatic_home_kit" "2"
"automatic_away_kit" "2"
"automatic_quit_when_done" "true"
"match_duration_minutes" "5"

"dataset_export_enabled" "true"
"dataset_export_root" "output/datasets/soccerreplay1988"
```

`automatic_match_count` is the number of matches to acquire. With
`automatic_random_teams=true`, two distinct teams are selected from the database
before every match; the configured team IDs are ignored, and the same pairing
is not repeated twice in a row even with home and away reversed. Set it to `false` to
reuse the two fixed IDs for the whole batch. Every match gets its own dataset
directory containing `annotations.json`, `frame_index.json`, `frames/`, and
event clips. `annotations.json` includes the complete 24-label SoccerReplay-1988
taxonomy in `event_labels`, in addition to the events actually observed in
`event_descriptions`. The application exits after the final exporter flush when
`automatic_quit_when_done=true`; otherwise it returns to the main menu.

`match_duration_minutes` controls the total regulation-time simulation duration
(both halves combined); set pieces and stoppages can add a small amount of wall
time. The legacy normalized `match_duration` value is used only when the new
setting is absent. Existing tracking exporters remain configurable through
`blender_tracking_export_*` and `cosmos_capture_*`.

Set `match_extra_time_enabled=false` to finish after the second half regardless
of the score. The default is `true` for compatibility with older configurations.

The sample `data/football.config` is organized by responsibility. `random_seed`
controls every simulator random source: use `-1` for a new run based on the
high-resolution clock, or a non-negative integer to reproduce a run (including random teams,
player appearance and lighting). `match_lighting` accepts `random`, `day`, or
`night`; the latter two disable lighting variation so generated captures are
consistent.

`ai_offensive_aggression` controls how readily AI teams attempt progressive
passes and assists. `1.0` is the conservative baseline; values around `1.25` to
`1.5` encourage vertical play. Shot selection is controlled separately by
`ai_shot_max_distance` and `ai_shot_min_quality`; quality accounts for distance,
angle, shooting lane, nearby pressure, body and movement balance, ball stability,
and shooting technique. A clear pass to a better scoring position is preferred
over the shot. `ai_final_third_decision_ms` prevents attackers from holding the
ball until the chance disappears, while `ai_shot_accuracy_assist` reduces the
animation-layer error only after the AI has selected a credible shot;
`ai_shot_height` controls the low trajectory used for those finishes. The
aggression range is `0.5` to `2.0`.

Pass selection also rejects routine passes from an outfield player to its own
goalkeeper, strongly penalizes backward recycling in attacking areas, and keeps
a short pass history to prevent immediate two-player ping-pong. From wide final-
third positions, a reachable teammate in the box is treated as a crossing
target and receives a high pass; attackers and wide midfielders make supporting
runs into the area instead of waiting outside it.

Open-play passing now checks the projected offside line at ball contact and
excludes receivers who are clearly beyond it, while off-ball runners leave a
small acceleration margin. Reliable forward passes gain priority over purely
lateral circulation. Central free kicks 17–29 meters from goal are taken as
direct shots; wide attacking free kicks and corners seek an in-box aerial
receiver with heading ability. The optional 30-meter shot limit still requires
balance, a clear lane and shooting skill for attempts from outside the box.

`ai_cross_aggression` raises or lowers the preference for genuine high crosses
from wide attacking positions. `ai_defensive_challenge_aggression` controls how
readily CPU defenders attempt a tackle, while `referee_foul_sensitivity`
controls how readily physical contacts are called. Together these settings can
increase corners, free kicks and penalties through actual play; they never emit
an event label unless the corresponding restart is awarded by the referee.

Set `ai_automatic_substitutions=true` to replace the most fatigued outfield
player on each team at the start of the second half. The number of changes per
team is controlled by `ai_halftime_substitutions_per_team` (0 to 3). These are
real squad changes for gameplay statistics and fatigue and emit the SoccerReplay
`substitution` event.

Shot outcomes are connected to the SoccerReplay exporter: goalkeeper touches,
goal-line restarts and corners generate `saved by goal-keeper`, `shot off
target`, `corner`, and `lead to corner` annotations as appropriate.
Actual fouls, cards, penalties, free kicks, off-sides, defensive clearances,
balls out of play and the full-time summary are exported as their canonical
SoccerReplay-1988 labels as well.

The segmentation control video labels visible painted field markings as
`field_lines` in magenta `[255,0,255]`. This includes the touchlines, goal
lines, halfway line, center circle, penalty and goal areas, penalty arcs,
corner arcs, and spots already present in the pitch texture. The markings
are classified only on the pitch surface, so players and the ball retain
their existing labels when they occlude a line. `metadata.json` records the
new color in `semantic_palette`.

`cosmos_segmentation_v2=true` also labels stadium submeshes by material during
the semantic render pass: advertising boards `[255,128,0]`, barriers and walls
`[0,128,128]`, crowd-textured seating `[128,0,128]`, and stands structure
`[160,96,48]`. Existing team, official, ball, pitch, and field-line colors are
unchanged. The crowd class follows the stadium's crowd-textured meshes; it
does not distinguish individual spectators. `metadata.json` includes
`segmentation_version` and the full palette. Set the option to `false` to
export the original single-color stadium mask for an A/B comparison with the
same seed. `cosmos_capture_prompt` controls the caption written to `prompt.json`;
event-specific captures can override it in their own config file. Keep the
lossless MKV segmentation master when exact palette values matter.

Depth capture defaults to a global metric mapping. The renderer reconstructs
camera-space Z from the OpenGL depth buffer using each frame's projection near
and far planes, then maps the fixed interval configured by
`cosmos_depth_near_m` and `cosmos_depth_far_m` to 8-bit grayscale. Near remains
bright and far remains dark. The fixed interval is shared by all frames and
cameras; no per-frame normalization is applied. Set
`cosmos_depth_mapping=legacy_device` only to reproduce the earlier inverted
device-depth export.


## Problems? 
If you have any problems please open an issue. 


### Donate
If you want to thank Bastiaan for his great work, consider a donation to his Bitcoin address 1JHnTe2QQj8RL281fXFiyvK9igj2VhPh2t
