#include "camera_director.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <sstream>

namespace {

const float kPi = 3.14159265358979323846f;

std::string Upper(std::string value) {
  for (std::size_t i = 0; i < value.size(); ++i) {
    if (value[i] >= 'a' && value[i] <= 'z') value[i] = value[i] - 'a' + 'A';
  }
  return value;
}

float ClampValue(float value, float minimum, float maximum) {
  return std::max(minimum, std::min(maximum, value));
}

float WrapAngle(float angle) {
  while (angle > kPi) angle -= 2.0f * kPi;
  while (angle < -kPi) angle += 2.0f * kPi;
  return angle;
}

float SmoothFactor(float response, unsigned long delta_ms) {
  if (delta_ms == 0) return 0.0f;
  return 1.0f - std::exp(-response * static_cast<float>(delta_ms) * 0.001f);
}

CameraPreset MakePreset(e_CameraPresetType type, const char *name,
                        float anchorX, float anchorY, float height,
                        float fov, float minFov, float maxFov,
                        float maxPan, float minTilt, float maxTilt,
                        float trackingResponse, float zoomResponse,
                        float weight, float minShot, float maxShot,
                        e_CameraTrackingMode tracking) {
  CameraPreset result;
  result.type = type;
  result.name = name;
  result.anchorX = anchorX;
  result.anchorY = anchorY;
  result.height = height;
  result.fov = fov;
  result.minFov = minFov;
  result.maxFov = maxFov;
  result.maxPanDegrees = maxPan;
  result.minTiltDegrees = minTilt;
  result.maxTiltDegrees = maxTilt;
  result.trackingResponse = trackingResponse;
  result.zoomResponse = zoomResponse;
  result.weight = weight;
  result.minShotSeconds = minShot;
  result.maxShotSeconds = maxShot;
  result.tracking = tracking;
  return result;
}

}  // namespace

CameraDirector::CameraDirector(Properties *configuration)
    : mode(e_Mode_Legacy), enabled(false), closeUpEnabled(false), initialized(false), finalThirdActive(false),
      randomState(1), cameraId(0), maxCutsPerClip(1), cutsInClip(0),
      clipDuration_ms(0), directorStart_ms(0), clipStart_ms(0), shotStart_ms(0), shotEnd_ms(0),
      singlePreset(e_CameraPreset_MainBroadcast), activePreset(e_CameraPreset_Invalid),
      activeAnchor(0), smoothedFocus(0), smoothedFov(35.0f) {
  BuildPresets(configuration);

  std::string modeName = Upper(configuration->Get("camera_mode", "single"));
  if (modeName == "LEGACY") mode = e_Mode_Legacy;
  else if (modeName == "SCHEDULED") mode = e_Mode_Scheduled;
  else if (modeName == "PROCEDURAL" || modeName == "RANDOMIZED") mode = e_Mode_Procedural;
  else mode = e_Mode_Single;

  if (configuration->GetBool("single_camera", false)) mode = e_Mode_Single;
  if (!configuration->GetBool("camera_changes", true) && mode != e_Mode_Legacy) mode = e_Mode_Single;
  enabled = mode != e_Mode_Legacy;
  closeUpEnabled = configuration->GetBool("camera_close_up_enabled", false);
  singlePreset = ParsePresetType(configuration->Get("camera_single_preset", "MAIN_BROADCAST"));
  if (singlePreset == e_CameraPreset_Invalid) singlePreset = e_CameraPreset_MainBroadcast;

  int configuredSeed = configuration->GetInt("camera_seed", -1);
  if (configuredSeed < 0) configuredSeed = configuration->GetInt("random_seed", -1);
  if (configuredSeed < 0) configuredSeed = static_cast<int>(std::time(0));
  randomState = static_cast<unsigned int>(configuredSeed) ^ 0x9e3779b9u;
  if (randomState == 0) randomState = 1;

  maxCutsPerClip = std::max(0, configuration->GetInt("camera_max_cuts_per_clip", 1));
  clipDuration_ms = static_cast<unsigned long>(std::max(0.0f,
      configuration->GetReal("camera_clip_duration_s", 0.0f)) * 1000.0f);
  ParseSchedule(configuration->Get("camera_schedule", "0:MAIN_BROADCAST"));
}

void CameraDirector::BuildPresets(Properties *configuration) {
  presets[e_CameraPreset_MainBroadcast] = MakePreset(
      e_CameraPreset_MainBroadcast, "MAIN_BROADCAST", 0.0f, -1.18f, 0.58f,
      34.0f, 29.0f, 44.0f, 62.0f, 14.0f, 82.0f, 2.2f, 1.2f, 10.0f, 6.0f, 14.0f,
      e_CameraTracking_Action);
  presets[e_CameraPreset_HighWide] = MakePreset(
      e_CameraPreset_HighWide, "HIGH_WIDE", 0.0f, -1.12f, 1.10f,
      45.0f, 40.0f, 54.0f, 58.0f, 10.0f, 84.0f, 1.5f, 0.8f, 4.0f, 5.0f, 12.0f,
      e_CameraTracking_Tactical);
  presets[e_CameraPreset_HighGoalSide] = MakePreset(
      e_CameraPreset_HighGoalSide, "HIGH_GOAL_SIDE", 0.48f, -1.00f, 0.66f,
      38.0f, 33.0f, 50.0f, 52.0f, 10.0f, 84.0f, 2.0f, 1.0f, 4.0f, 4.0f, 9.0f,
      e_CameraTracking_GoalArea);
  presets[e_CameraPreset_LowTouchline] = MakePreset(
      e_CameraPreset_LowTouchline, "LOW_TOUCHLINE", 0.0f, -1.015f, 0.095f,
      39.0f, 33.0f, 52.0f, 74.0f, 35.0f, 88.0f, 2.8f, 1.4f, 1.5f, 3.0f, 6.0f,
      e_CameraTracking_Action);
  presets[e_CameraPreset_LowGoalSide] = MakePreset(
      e_CameraPreset_LowGoalSide, "LOW_GOAL_SIDE", 0.72f, -1.015f, 0.11f,
      31.0f, 25.0f, 44.0f, 82.0f, 25.0f, 88.0f, 3.0f, 1.6f, 1.0f, 3.0f, 6.0f,
      e_CameraTracking_GoalArea);
  presets[e_CameraPreset_Corner] = MakePreset(
      e_CameraPreset_Corner, "CORNER", 0.99f, -0.99f, 0.06f,
      42.0f, 36.0f, 50.0f, 55.0f, 40.0f, 86.0f, 3.0f, 1.5f, 0.5f, 2.0f, 5.0f,
      e_CameraTracking_GoalArea);
  presets[e_CameraPreset_BehindGoal] = MakePreset(
      e_CameraPreset_BehindGoal, "BEHIND_GOAL", 1.04f, 0.0f, 0.20f,
      42.0f, 35.0f, 52.0f, 55.0f, 20.0f, 84.0f, 2.4f, 1.2f, 0.7f, 3.0f, 7.0f,
      e_CameraTracking_GoalArea);
  presets[e_CameraPreset_CloseUp] = MakePreset(
      e_CameraPreset_CloseUp, "CLOSE_UP", 0.0f, 0.0f, 0.075f,
      24.0f, 18.0f, 30.0f, 90.0f, 65.0f, 88.0f, 4.0f, 2.0f, 0.15f, 2.0f, 4.0f,
      e_CameraTracking_Player);

  const e_CameraPresetType types[] = {
      e_CameraPreset_MainBroadcast, e_CameraPreset_HighWide,
      e_CameraPreset_HighGoalSide, e_CameraPreset_LowTouchline,
      e_CameraPreset_LowGoalSide, e_CameraPreset_Corner,
      e_CameraPreset_BehindGoal, e_CameraPreset_CloseUp};
  for (unsigned int i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
    CameraPreset &preset = presets[types[i]];
    const std::string prefix = "camera_" + Upper(preset.name) + "_";
    preset.weight = configuration->GetReal((prefix + "weight").c_str(), preset.weight);
    preset.minShotSeconds = configuration->GetReal((prefix + "min_shot_s").c_str(), preset.minShotSeconds);
    preset.maxShotSeconds = configuration->GetReal((prefix + "max_shot_s").c_str(), preset.maxShotSeconds);
    preset.fov = configuration->GetReal((prefix + "fov").c_str(), preset.fov);
  }
}

void CameraDirector::ParseSchedule(const std::string &value) {
  schedule.clear();
  std::stringstream stream(value);
  std::string token;
  while (std::getline(stream, token, ',')) {
    const std::size_t separator = token.find(':');
    if (separator == std::string::npos) continue;
    const float seconds = static_cast<float>(std::atof(token.substr(0, separator).c_str()));
    const e_CameraPresetType preset = ParsePresetType(token.substr(separator + 1));
    if (preset == e_CameraPreset_Invalid) continue;
    ScheduleEntry entry;
    entry.start_ms = static_cast<unsigned long>(std::max(0.0f, seconds) * 1000.0f);
    entry.preset = preset;
    schedule.push_back(entry);
  }
  std::sort(schedule.begin(), schedule.end(),
      [](const ScheduleEntry &a, const ScheduleEntry &b) { return a.start_ms < b.start_ms; });
}

e_CameraPresetType CameraDirector::ParsePresetType(const std::string &name) {
  const std::string value = Upper(name);
  if (value.find("MAIN_BROADCAST") != std::string::npos) return e_CameraPreset_MainBroadcast;
  if (value.find("HIGH_WIDE") != std::string::npos) return e_CameraPreset_HighWide;
  if (value.find("HIGH_GOAL_SIDE") != std::string::npos || value.find("HIGH_DIAGONAL") != std::string::npos) return e_CameraPreset_HighGoalSide;
  if (value.find("LOW_TOUCHLINE") != std::string::npos) return e_CameraPreset_LowTouchline;
  if (value.find("LOW_GOAL_SIDE") != std::string::npos || value.find("LOW_FIELD_LEVEL") != std::string::npos) return e_CameraPreset_LowGoalSide;
  if (value.find("BEHIND_GOAL") != std::string::npos) return e_CameraPreset_BehindGoal;
  if (value.find("CLOSE_UP") != std::string::npos || value.find("PLAYER") != std::string::npos) return e_CameraPreset_CloseUp;
  if (value.find("CORNER") != std::string::npos) return e_CameraPreset_Corner;
  return e_CameraPreset_Invalid;
}

const char *CameraDirector::PresetName(e_CameraPresetType type) {
  switch (type) {
    case e_CameraPreset_MainBroadcast: return "MAIN_BROADCAST";
    case e_CameraPreset_HighWide: return "HIGH_WIDE";
    case e_CameraPreset_HighGoalSide: return "HIGH_GOAL_SIDE";
    case e_CameraPreset_LowTouchline: return "LOW_TOUCHLINE";
    case e_CameraPreset_LowGoalSide: return "LOW_GOAL_SIDE";
    case e_CameraPreset_Corner: return "CORNER";
    case e_CameraPreset_BehindGoal: return "BEHIND_GOAL";
    case e_CameraPreset_CloseUp: return "CLOSE_UP";
    default: return "LEGACY";
  }
}

float CameraDirector::Random01() {
  // xorshift32 gives the same sequence on every supported compiler.
  randomState ^= randomState << 13;
  randomState ^= randomState >> 17;
  randomState ^= randomState << 5;
  return static_cast<float>(randomState & 0x00ffffffu) / 16777216.0f;
}

float CameraDirector::RandomRange(float minimum, float maximum) {
  return minimum + (maximum - minimum) * Random01();
}

float CameraDirector::GetContextWeight(e_CameraPresetType type, const CameraDirectorInput &input) const {
  const float goalProximity = ClampValue((std::fabs(input.ballPosition.coords[0]) / pitchHalfW - 0.45f) / 0.45f, 0.0f, 1.0f);
  const float wingProximity = ClampValue((std::fabs(input.ballPosition.coords[1]) / pitchHalfH - 0.48f) / 0.42f, 0.0f, 1.0f);
  const float isolation = ClampValue((8.0f - static_cast<float>(input.nearbyPlayerCount)) / 5.0f, 0.0f, 1.0f);
  const bool finalThirdPosition = std::fabs(input.ballPosition.coords[0]) >= pitchHalfW - 25.0f;
  const bool corner = input.setPieceActive && input.setPiece == e_SetPiece_Corner;
  const bool throwIn = input.setPieceActive && input.setPiece == e_SetPiece_ThrowIn;
  const bool stoppage = !input.inPlay;
  switch (type) {
    case e_CameraPreset_MainBroadcast: return 1.0f;
    case e_CameraPreset_HighWide: return corner ? 1.2f : std::max(0.04f, 1.0f - goalProximity * 1.8f);
    case e_CameraPreset_HighGoalSide:
      return finalThirdPosition ? 1.0f + goalProximity * 4.0f + (input.setPieceActive ? 1.2f : 0.0f) : 0.0f;
    case e_CameraPreset_LowTouchline:
      if (corner) return 0.0f;
      if (throwIn) return 6.0f;
      return input.inPlay ? wingProximity * isolation * 1.4f : 0.0f;
    case e_CameraPreset_LowGoalSide: return 0.15f + goalProximity * 1.8f;
    case e_CameraPreset_Corner: return corner ? 9.0f : 0.0f;
    case e_CameraPreset_BehindGoal: return corner ? 2.2f : (goalProximity * 0.5f);
    case e_CameraPreset_CloseUp: return closeUpEnabled && stoppage ? 1.0f : 0.0f;
    default: return 0.0f;
  }
}

e_CameraPresetType CameraDirector::SelectPreset(const CameraDirectorInput &input) {
  if (mode == e_Mode_Single) return singlePreset;
  if (mode == e_Mode_Scheduled) {
    e_CameraPresetType result = schedule.empty() ? singlePreset : schedule.front().preset;
    const unsigned long elapsed_ms = input.time_ms - directorStart_ms;
    for (std::size_t i = 0; i < schedule.size(); ++i) {
      if (schedule[i].start_ms > elapsed_ms) break;
      result = schedule[i].preset;
    }
    return result;
  }

  float totalWeight = 0.0f;
  for (std::map<e_CameraPresetType, CameraPreset>::const_iterator it = presets.begin(); it != presets.end(); ++it) {
    if (it->first == activePreset && presets.size() > 1) continue;
    totalWeight += std::max(0.0f, it->second.weight * GetContextWeight(it->first, input));
  }
  if (totalWeight <= 0.0f) return e_CameraPreset_MainBroadcast;
  float choice = Random01() * totalWeight;
  for (std::map<e_CameraPresetType, CameraPreset>::const_iterator it = presets.begin(); it != presets.end(); ++it) {
    if (it->first == activePreset && presets.size() > 1) continue;
    choice -= std::max(0.0f, it->second.weight * GetContextWeight(it->first, input));
    if (choice <= 0.0f) return it->first;
  }
  return e_CameraPreset_MainBroadcast;
}

Vector3 CameraDirector::BuildFocus(const CameraPreset &preset, const CameraDirectorInput &input) const {
  Vector3 focus = input.ballPosition * 0.78f + input.nearbyPlayersCenter * 0.22f;
  focus += input.actionDirection.GetNormalized(Vector3(0)) * 1.2f;
  focus.coords[2] = preset.tracking == e_CameraTracking_Player ? 1.15f : 0.8f;
  if (preset.tracking == e_CameraTracking_Tactical) {
    focus = input.ballPosition * 0.65f + input.nearbyPlayersCenter * 0.35f;
    focus.coords[2] = 0.5f;
  } else if (preset.tracking == e_CameraTracking_GoalArea) {
    // Keep the goal selected at the cut for the whole shot. Following the
    // instantaneous ball sign would swing through midfield on a clearance.
    const float side = activeAnchor.coords[0] < 0.0f ? -1.0f : 1.0f;
    Vector3 goalFocus(side * pitchHalfW * 0.88f, 0, 0.9f);
    focus = input.ballPosition * 0.7f + goalFocus * 0.3f;
    if (preset.type == e_CameraPreset_Corner && input.setPieceActive &&
        input.setPiece == e_SetPiece_Corner) {
      // Preparation shot: keep the taker and ball central. The actual cross
      // is covered by HIGH_GOAL_SIDE just before the restart begins.
      focus = input.ballPosition * 0.65f + input.primaryPlayerPosition * 0.35f;
      focus.coords[2] = 1.15f;
    } else if (preset.type == e_CameraPreset_HighGoalSide) {
      // Keep enough of the penalty area in shot, especially the near-side
      // patch of grass directly below the physical camera.
      focus = input.ballPosition * 0.74f + goalFocus * 0.26f;
      // This preset is a final-third camera: never let a retreating attack
      // drag it across midfield or towards the opposite grandstand.
      focus.coords[0] = side * std::max(std::fabs(focus.coords[0]), pitchHalfW - 24.0f);
      focus.coords[1] = ClampValue(focus.coords[1], -pitchHalfH * 0.72f, pitchHalfH * 0.72f);
    }
  } else if (preset.tracking == e_CameraTracking_Player) {
    focus = input.primaryPlayerPosition + Vector3(0, 0, 1.1f);
  }
  focus.coords[0] = ClampValue(focus.coords[0], -pitchHalfW, pitchHalfW);
  focus.coords[1] = ClampValue(focus.coords[1], -pitchHalfH, pitchHalfH);
  return focus;
}

Vector3 CameraDirector::BuildAnchor(const CameraPreset &preset, const CameraDirectorInput &input) const {
  const float xSide = input.ballPosition.coords[0] < 0.0f ? -1.0f : 1.0f;
  const float ySide = input.ballPosition.coords[1] < 0.0f ? -1.0f : 1.0f;
  Vector3 anchor(preset.anchorX * pitchHalfW, preset.anchorY * pitchHalfH,
                 preset.height * pitchHalfH);
  if (preset.type == e_CameraPreset_HighGoalSide || preset.type == e_CameraPreset_LowGoalSide) {
    anchor.coords[0] *= xSide;
    anchor.coords[1] = std::fabs(anchor.coords[1]) * ySide;
  } else if (preset.type == e_CameraPreset_BehindGoal) {
    anchor.coords[0] *= xSide;
  } else if (preset.type == e_CameraPreset_LowTouchline) {
    anchor.coords[0] = ClampValue(input.ballPosition.coords[0], -pitchHalfW * 0.72f, pitchHalfW * 0.72f);
    anchor.coords[1] = std::fabs(preset.anchorY * pitchHalfH) * ySide;
  } else if (preset.type == e_CameraPreset_Corner) {
    const Vector3 corner(xSide * pitchHalfW, ySide * pitchHalfH, 0.0f);
    // A clean three-quarter preparation shot from inside the pitch. Keeping
    // the physical point inside avoids roofs, stands and advertising boards.
    anchor = corner + Vector3(-xSide * 15.0f, -ySide * 10.0f, 8.0f);
  } else if (preset.type == e_CameraPreset_CloseUp) {
    Vector3 direction = input.actionDirection.Get2D().GetNormalized(Vector3(1, 0, 0));
    Vector3 lateral(-direction.coords[1], direction.coords[0], 0);
    anchor = input.primaryPlayerPosition - direction * 7.0f + lateral * 2.5f + Vector3(0, 0, 2.7f);
  }
  return anchor;
}

void CameraDirector::ActivatePreset(e_CameraPresetType type, const CameraDirectorInput &input) {
  if (presets.find(type) == presets.end()) type = e_CameraPreset_MainBroadcast;
  activePreset = type;
  const CameraPreset &preset = presets[activePreset];
  activeAnchor = BuildAnchor(preset, input);
  smoothedFocus = BuildFocus(preset, input);
  smoothedFov = preset.fov;
  shotStart_ms = input.time_ms;
  shotEnd_ms = shotStart_ms + static_cast<unsigned long>(
      RandomRange(preset.minShotSeconds, std::max(preset.minShotSeconds, preset.maxShotSeconds)) * 1000.0f);
  ++cameraId;
}

CameraDirectorOutput CameraDirector::Update(const CameraDirectorInput &input) {
  bool cut = false;
  if (!initialized) {
    directorStart_ms = input.time_ms;
    clipStart_ms = input.time_ms;
    ActivatePreset(SelectPreset(input), input);
    initialized = true;
    cut = true;
  }

  if (clipDuration_ms > 0 && input.time_ms >= clipStart_ms + clipDuration_ms) {
    const unsigned long clipsElapsed = (input.time_ms - clipStart_ms) / clipDuration_ms;
    clipStart_ms += clipsElapsed * clipDuration_ms;
    cutsInClip = 0;
  }

  e_CameraPresetType requested = activePreset;
  if (mode == e_Mode_Scheduled || mode == e_Mode_Single) {
    requested = SelectPreset(input);
  } else if (mode == e_Mode_Procedural && input.time_ms >= shotEnd_ms &&
             cutsInClip < maxCutsPerClip) {
    requested = SelectPreset(input);
  }

  // Restarts need a guaranteed clean establishing angle, not just a larger
  // random-selection weight. Both presets mirror to the actual touchline.
  if (mode == e_Mode_Procedural && input.setPieceActive) {
    if (input.setPiece == e_SetPiece_Corner) {
      const bool kickImminent = input.setPieceStartTime_ms > 0 &&
          input.time_ms + 900 >= input.setPieceStartTime_ms;
      requested = kickImminent ? e_CameraPreset_HighGoalSide : e_CameraPreset_Corner;
    }
    else if (input.setPiece == e_SetPiece_ThrowIn) requested = e_CameraPreset_LowTouchline;
  }
  if (mode == e_Mode_Procedural && !input.setPieceActive &&
      activePreset == e_CameraPreset_Corner) {
    requested = e_CameraPreset_HighGoalSide;
  }

  // A generic tactical view should not survive deep into the attacking
  // third. Hysteresis avoids repeated cuts near the 25 metre boundary.
  const float absoluteBallX = std::fabs(input.ballPosition.coords[0]);
  const bool enteredFinalThird = !finalThirdActive && absoluteBallX >= pitchHalfW - 25.0f;
  if (finalThirdActive && absoluteBallX <= pitchHalfW - 30.0f) finalThirdActive = false;
  if (enteredFinalThird) {
    finalThirdActive = true;
  }
  if (mode == e_Mode_Procedural && !input.setPieceActive && finalThirdActive &&
      (activePreset == e_CameraPreset_MainBroadcast || activePreset == e_CameraPreset_HighWide) &&
      input.time_ms >= shotStart_ms + 2000) {
    requested = e_CameraPreset_HighGoalSide;
  }
  // Conversely, do not keep the goal-side angle after the attack has clearly
  // moved back out of the final third.
  if (mode == e_Mode_Procedural && !input.setPieceActive && !finalThirdActive &&
      activePreset == e_CameraPreset_HighGoalSide &&
      input.time_ms >= shotStart_ms + 3000) {
    requested = e_CameraPreset_MainBroadcast;
  }
  if (requested != activePreset) {
    ActivatePreset(requested, input);
    ++cutsInClip;
    cut = true;
  }

  const CameraPreset &preset = presets[activePreset];
  const Vector3 desiredFocus = BuildFocus(preset, input);
  if (!cut) {
    const float focusFactor = SmoothFactor(preset.trackingResponse, input.delta_ms);
    smoothedFocus = smoothedFocus * (1.0f - focusFactor) + desiredFocus * focusFactor;
  }

  const float finalThird = std::fabs(input.ballPosition.coords[0]) / pitchHalfW;
  const float speed = ClampValue(input.ballMovement.GetLength() / 18.0f, 0.0f, 1.0f);
  const float ballGroundDistance = (input.ballPosition - activeAnchor).Get2D().GetLength();
  const float nearCamera = ClampValue((28.0f - ballGroundDistance) / 22.0f, 0.0f, 1.0f);
  float desiredFov = preset.fov + speed * 2.5f - finalThird * 3.0f;
  if (activePreset == e_CameraPreset_HighGoalSide) {
    const float nearTouchline = std::fabs(input.ballPosition.coords[1]) / pitchHalfH;
    desiredFov = preset.fov + speed * 1.5f + nearTouchline * 3.0f;
  }
  // Widen every preset as play enters the patch directly below its physical
  // mount. This is the common blind zone of sideline and diagonal cameras.
  desiredFov += nearCamera * 8.0f;
  desiredFov = ClampValue(desiredFov, preset.minFov, preset.maxFov);
  if (!cut) {
    const float zoomFactor = SmoothFactor(preset.zoomResponse, input.delta_ms);
    smoothedFov += (desiredFov - smoothedFov) * zoomFactor;
  }

  // When the ball is close to the camera, editorial lead-room must yield to
  // ball retention. Blending here affects framing only; normal tracking
  // resumes smoothly as the play moves away.
  Vector3 framingFocus = smoothedFocus * (1.0f - nearCamera * 0.88f) +
                         input.ballPosition * (nearCamera * 0.88f);
  framingFocus.coords[2] = smoothedFocus.coords[2];
  const Vector3 toTarget = framingFocus - activeAnchor;
  const float centerYaw = (Vector3(0, 0, 0) - activeAnchor).GetAngle2D() + 1.5f * kPi;
  float yaw = toTarget.GetAngle2D() + 1.5f * kPi;
  const float maxPan = preset.maxPanDegrees * kPi / 180.0f;
  yaw = centerYaw + ClampValue(WrapAngle(yaw - centerYaw), -maxPan, maxPan);
  const float horizontalDistance = toTarget.Get2D().GetLength();
  float pitch = std::atan2(horizontalDistance, std::max(0.1f, activeAnchor.coords[2] - smoothedFocus.coords[2]));
  pitch = ClampValue(pitch, preset.minTiltDegrees * kPi / 180.0f,
                    preset.maxTiltDegrees * kPi / 180.0f);

  CameraDirectorOutput output;
  output.position = activeAnchor;
  output.yaw = yaw;
  output.pitch = pitch;
  output.fov = smoothedFov;
  output.nearCap = activePreset == e_CameraPreset_MainBroadcast || activePreset == e_CameraPreset_HighWide ? 2.0f : 0.25f;
  output.farCap = 260.0f;
  output.cut = cut;
  output.preset = activePreset;
  output.cameraName = PresetName(activePreset);
  output.cameraId = cameraId;
  return output;
}

std::string CameraDirector::GetActiveCameraName() const {
  return PresetName(activePreset);
}
