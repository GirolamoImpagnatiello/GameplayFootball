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
      previousSetPieceActive(false), previousSetPiece(e_SetPiece_None),
      randomState(1), cameraId(0), maxCutsPerClip(1), cutsInClip(0),
      clipDuration_ms(0), directorStart_ms(0), clipStart_ms(0), shotStart_ms(0), shotEnd_ms(0),
      cornerDeliveryUntil_ms(0),
      singlePreset(e_CameraPreset_MainBroadcast), activePreset(e_CameraPreset_Invalid),
      scriptedPreset(e_CameraPreset_Invalid),
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

bool CameraDirector::SetScriptedPreset(const std::string &name) {
  const std::string value = Upper(name);
  if (value == "AUTO" || value == "NONE") {
    scriptedPreset = e_CameraPreset_Invalid;
    return true;
  }
  const e_CameraPresetType parsed = ParsePresetType(value);
  if (parsed == e_CameraPreset_Invalid) return false;
  scriptedPreset = parsed;
  return true;
}

void CameraDirector::BuildPresets(Properties *configuration) {
  presets[e_CameraPreset_MainBroadcast] = MakePreset(
      e_CameraPreset_MainBroadcast, "MAIN_BROADCAST", 0.0f, -1.18f, 0.58f,
      34.0f, 29.0f, 44.0f, 62.0f, 14.0f, 82.0f, 2.2f, 1.2f, 10.0f, 6.0f, 14.0f,
      e_CameraTracking_Action);
  presets[e_CameraPreset_HighWide] = MakePreset(
      e_CameraPreset_HighWide, "HIGH_WIDE", 0.0f, -1.20f, 0.92f,
      48.0f, 43.0f, 56.0f, 62.0f, 12.0f, 84.0f, 1.7f, 0.9f, 4.0f, 5.0f, 12.0f,
      e_CameraTracking_Tactical);
  // Single-camera television master. The mount sits a little farther behind
  // the near touchline and travels only part of the ball's longitudinal
  // movement. The remaining offset preserves a diagonal broadcast view even
  // while the camera follows play toward either goal.
  presets[e_CameraPreset_CosmosWide] = MakePreset(
      e_CameraPreset_CosmosWide, "COSMOS_WIDE", 0.0f, -1.22f, 0.42f,
      27.0f, 17.0f, 40.0f, 62.0f, 14.0f, 82.0f, 2.0f, 1.35f, 0.0f, 3600.0f, 3600.0f,
      e_CameraTracking_Action);
  // Fixed lateral diagnostic view for temporal-consistency tests. It keeps
  // the visual language of MAIN_BROADCAST. A moderately wide lens lets the
  // mount sit closer and lower than the first version while retaining both
  // goals and all four touchline corners.
  presets[e_CameraPreset_FullFieldDebug] = MakePreset(
      e_CameraPreset_FullFieldDebug, "FULL_FIELD_DEBUG", 0.0f, -2.11f, 0.89f,
      68.0f, 68.0f, 68.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 3600.0f, 3600.0f,
      e_CameraTracking_Tactical);
  // Original stadium-wide lateral composition retained as a separate option.
  // Unlike FULL_FIELD_DEBUG, this view deliberately includes more of the
  // stands and uses a longer, less distorted lens.
  presets[e_CameraPreset_FullFieldPanoramic] = MakePreset(
      e_CameraPreset_FullFieldPanoramic, "FULL_FIELD_PANORAMIC", 0.0f, -2.92f, 1.53f,
      52.0f, 52.0f, 52.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 3600.0f, 3600.0f,
      e_CameraTracking_Tactical);
  presets[e_CameraPreset_HighGoalSide] = MakePreset(
      e_CameraPreset_HighGoalSide, "HIGH_GOAL_SIDE", 0.40f, -1.14f, 0.74f,
      42.0f, 37.0f, 55.0f, 66.0f, 12.0f, 84.0f, 2.8f, 1.3f, 4.0f, 4.0f, 9.0f,
      e_CameraTracking_GoalArea);
  presets[e_CameraPreset_LowTouchline] = MakePreset(
      e_CameraPreset_LowTouchline, "LOW_TOUCHLINE", 0.0f, -1.06f, 0.23f,
      46.0f, 40.0f, 56.0f, 86.0f, 28.0f, 86.0f, 3.2f, 1.6f, 1.5f, 3.0f, 6.0f,
      e_CameraTracking_Action);
  presets[e_CameraPreset_LowGoalSide] = MakePreset(
      e_CameraPreset_LowGoalSide, "LOW_GOAL_SIDE", 0.78f, -0.82f, 0.24f,
      36.0f, 31.0f, 47.0f, 74.0f, 38.0f, 86.0f, 3.0f, 1.6f, 1.0f, 3.0f, 6.0f,
      e_CameraTracking_GoalArea);
  presets[e_CameraPreset_Corner] = MakePreset(
      e_CameraPreset_Corner, "CORNER", 0.99f, -0.99f, 0.06f,
      70.0f, 62.0f, 78.0f, 180.0f, 10.0f, 84.0f, 4.0f, 2.0f, 0.5f, 2.0f, 5.0f,
      e_CameraTracking_GoalArea);
  presets[e_CameraPreset_Penalty] = MakePreset(
      e_CameraPreset_Penalty, "PENALTY", 0.0f, 0.0f, 0.0f,
      58.0f, 50.0f, 66.0f, 180.0f, 10.0f, 78.0f, 4.5f, 2.0f, 0.0f, 2.0f, 6.0f,
      e_CameraTracking_GoalArea);
  presets[e_CameraPreset_FreeKick] = MakePreset(
      e_CameraPreset_FreeKick, "FREE_KICK", 0.0f, 0.0f, 0.0f,
      60.0f, 52.0f, 68.0f, 180.0f, 10.0f, 80.0f, 4.0f, 1.8f, 0.0f, 2.0f, 6.0f,
      e_CameraTracking_GoalArea);
  presets[e_CameraPreset_BehindGoal] = MakePreset(
      e_CameraPreset_BehindGoal, "BEHIND_GOAL", 1.0f, 0.0f, 0.35f,
      40.0f, 35.0f, 50.0f, 58.0f, 30.0f, 84.0f, 2.4f, 1.1f, 0.7f, 3.0f, 7.0f,
      e_CameraTracking_GoalArea);
  presets[e_CameraPreset_CloseUp] = MakePreset(
      e_CameraPreset_CloseUp, "CLOSE_UP", 0.0f, 0.0f, 0.075f,
      24.0f, 18.0f, 30.0f, 90.0f, 65.0f, 88.0f, 4.0f, 2.0f, 0.15f, 2.0f, 4.0f,
      e_CameraTracking_Player);

  const e_CameraPresetType types[] = {
      e_CameraPreset_MainBroadcast, e_CameraPreset_HighWide,
      e_CameraPreset_CosmosWide, e_CameraPreset_FullFieldDebug,
      e_CameraPreset_FullFieldPanoramic,
      e_CameraPreset_HighGoalSide, e_CameraPreset_LowTouchline,
      e_CameraPreset_LowGoalSide, e_CameraPreset_Corner,
      e_CameraPreset_Penalty, e_CameraPreset_FreeKick,
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
  if (value.find("FULL_FIELD_DEBUG") != std::string::npos ||
      value.find("FULL_PITCH_DEBUG") != std::string::npos) return e_CameraPreset_FullFieldDebug;
  if (value.find("FULL_FIELD_PANORAMIC") != std::string::npos ||
      value.find("STADIUM_WIDE") != std::string::npos) return e_CameraPreset_FullFieldPanoramic;
  if (value.find("COSMOS_WIDE") != std::string::npos) return e_CameraPreset_CosmosWide;
  if (value.find("HIGH_WIDE") != std::string::npos) return e_CameraPreset_HighWide;
  if (value.find("HIGH_GOAL_SIDE") != std::string::npos || value.find("HIGH_DIAGONAL") != std::string::npos) return e_CameraPreset_HighGoalSide;
  if (value.find("LOW_TOUCHLINE") != std::string::npos) return e_CameraPreset_LowTouchline;
  if (value.find("LOW_GOAL_SIDE") != std::string::npos || value.find("LOW_FIELD_LEVEL") != std::string::npos) return e_CameraPreset_LowGoalSide;
  if (value.find("BEHIND_GOAL") != std::string::npos) return e_CameraPreset_BehindGoal;
  if (value.find("PENALTY") != std::string::npos) return e_CameraPreset_Penalty;
  if (value.find("FREE_KICK") != std::string::npos || value.find("FREEKICK") != std::string::npos) return e_CameraPreset_FreeKick;
  if (value.find("CLOSE_UP") != std::string::npos || value.find("PLAYER") != std::string::npos) return e_CameraPreset_CloseUp;
  if (value.find("CORNER") != std::string::npos) return e_CameraPreset_Corner;
  return e_CameraPreset_Invalid;
}

const char *CameraDirector::PresetName(e_CameraPresetType type) {
  switch (type) {
    case e_CameraPreset_MainBroadcast: return "MAIN_BROADCAST";
    case e_CameraPreset_HighWide: return "HIGH_WIDE";
    case e_CameraPreset_CosmosWide: return "COSMOS_WIDE";
    case e_CameraPreset_FullFieldDebug: return "FULL_FIELD_DEBUG";
    case e_CameraPreset_FullFieldPanoramic: return "FULL_FIELD_PANORAMIC";
    case e_CameraPreset_HighGoalSide: return "HIGH_GOAL_SIDE";
    case e_CameraPreset_LowTouchline: return "LOW_TOUCHLINE";
    case e_CameraPreset_LowGoalSide: return "LOW_GOAL_SIDE";
    case e_CameraPreset_Corner: return "CORNER";
    case e_CameraPreset_Penalty: return "PENALTY";
    case e_CameraPreset_FreeKick: return "FREE_KICK";
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
  const bool penalty = input.setPieceActive && input.setPiece == e_SetPiece_Penalty;
  const bool freeKick = input.setPieceActive && input.setPiece == e_SetPiece_FreeKick;
  const bool stoppage = !input.inPlay;
  switch (type) {
    case e_CameraPreset_MainBroadcast: return 1.0f;
    case e_CameraPreset_HighWide: return corner ? 1.2f : std::max(0.04f, 1.0f - goalProximity * 1.8f);
    case e_CameraPreset_CosmosWide: return 0.0f;
    case e_CameraPreset_FullFieldDebug: return 0.0f;
    case e_CameraPreset_FullFieldPanoramic: return 0.0f;
    case e_CameraPreset_HighGoalSide:
      return finalThirdPosition ? 1.0f + goalProximity * 4.0f + (input.setPieceActive ? 1.2f : 0.0f) : 0.0f;
    case e_CameraPreset_LowTouchline:
      if (corner) return 0.0f;
      if (throwIn) return 6.0f;
      return input.inPlay ? wingProximity * isolation * 1.4f : 0.0f;
    case e_CameraPreset_LowGoalSide:
      // This low angle is intended for live open play only. In particular it
      // must never compete with the dedicated CORNER/HIGH_GOAL_SIDE sequence
      // or appear around midfield merely because of a non-zero base weight.
      return input.inPlay && !input.setPieceActive && finalThirdPosition
          ? 0.35f + goalProximity * 1.8f : 0.0f;
    case e_CameraPreset_Corner: return corner ? 9.0f : 0.0f;
    case e_CameraPreset_Penalty: return penalty ? 10.0f : 0.0f;
    case e_CameraPreset_FreeKick: return freeKick ? 10.0f : 0.0f;
    case e_CameraPreset_BehindGoal:
      // An occasional reverse angle for live attacks in the final third.
      // It is not a restart camera and must not linger once play moves away.
      return input.inPlay && !input.setPieceActive && finalThirdPosition
          ? goalProximity * 0.55f : 0.0f;
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
  const float actionSpeed = ClampValue(input.ballMovement.GetLength() / 18.0f, 0.0f, 1.0f);
  // Fast television pans look ahead of the ball. This both exposes the next
  // passing lane and prevents a sprint or clearance from reaching the edge
  // of frame before the mechanically smoothed camera can catch up.
  const float leadDistance = 1.2f + actionSpeed * 3.2f;
  focus += input.actionDirection.GetNormalized(Vector3(0)) * leadDistance;
  focus.coords[2] = preset.tracking == e_CameraTracking_Player ? 1.15f : 0.8f;
  if (preset.type == e_CameraPreset_CosmosWide) {
    // A television operator frames the phase of play, not only the ball. In
    // the attacking third, bias the optical centre toward the goal so that
    // the goalmouth and most of the box remain readable like a real master
    // shot. The blend fades out through midfield to avoid premature panning.
    const float attackingSide = input.ballPosition.coords[0] < 0.0f ? -1.0f : 1.0f;
    const float attackDepth = ClampValue(
        (std::fabs(input.ballPosition.coords[0]) / pitchHalfW - 0.45f) / 0.45f,
        0.0f, 1.0f);
    const Vector3 goalContext(attackingSide * (pitchHalfW - 5.5f), 0.0f, 0.8f);
    const float contextBlend = attackDepth * 0.36f;
    focus = focus * (1.0f - contextBlend) + goalContext * contextBlend;
    const float cameraSide = preset.anchorY < 0.0f ? -1.0f : 1.0f;
    const float nearTouchline = ClampValue(
        (cameraSide * input.ballPosition.coords[1] / pitchHalfH - 0.45f) / 0.50f,
        0.0f, 1.0f);
    const float goalLineDepth = ClampValue(
        (std::fabs(input.ballPosition.coords[0]) / pitchHalfW - 0.55f) / 0.40f,
        0.0f, 1.0f);
    // Near-side play is framed a few metres infield. This keeps supporting
    // players in view and prevents the camera from pointing almost straight
    // down at a ball directly below the stand.
    focus.coords[1] -= cameraSide * nearTouchline *
        (5.0f - goalLineDepth * 1.5f);
    const bool sameSideCorner = input.setPieceActive &&
        input.setPiece == e_SetPiece_Corner && nearTouchline > 0.65f;
    if (sameSideCorner) {
      // Compose between the taker and the penalty-area group instead of
      // centring either subject. With the wide corner lens this retains the
      // complete delivery path in one continuous master shot.
      const Vector3 penaltySpot(input.attackingGoalSide * (pitchHalfW - 11.0f),
                                0.0f, 1.0f);
      focus = input.ballPosition * 0.55f + penaltySpot * 0.45f;
    }
    // Aim slightly above the old ball-height target. This lifts the optical
    // axis without sacrificing the grass immediately around the action.
    focus.coords[2] = 1.2f;
  } else if (preset.type == e_CameraPreset_Penalty) {
    const float goalSide = input.attackingGoalSide;
    const Vector3 goal(goalSide * pitchHalfW, 0.0f, 1.15f);
    focus = input.ballPosition * 0.42f + goal * 0.58f;
    focus.coords[2] = 1.05f;
  } else if (preset.type == e_CameraPreset_FreeKick) {
    const float goalSide = input.attackingGoalSide;
    const Vector3 goal(goalSide * pitchHalfW, 0.0f, 1.25f);
    focus = input.ballPosition * 0.35f + goal * 0.65f;
    focus.coords[2] = 1.15f;
  } else if (preset.tracking == e_CameraTracking_Tactical) {
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
      // Look over the taker's shoulder into the penalty area. The actual
      // cross is covered by HIGH_GOAL_SIDE just before the restart begins.
      const Vector3 penaltySpot(input.attackingGoalSide * (pitchHalfW - 11.0f), 0.0f, 0.0f);
      focus = input.ballPosition * 0.75f + penaltySpot * 0.25f;
      focus.coords[2] = 0.8f;
    } else if (preset.type == e_CameraPreset_HighGoalSide) {
      // Keep enough of the penalty area in shot, especially the near-side
      // patch of grass directly below the physical camera.
      focus = input.ballPosition * 0.84f + goalFocus * 0.16f;
      // This preset is a final-third camera: never let a retreating attack
      // drag it across midfield or towards the opposite grandstand.
      focus.coords[0] = side * std::max(std::fabs(focus.coords[0]), pitchHalfW - 24.0f);
      focus.coords[1] = ClampValue(focus.coords[1], -pitchHalfH * 0.94f, pitchHalfH * 0.94f);
    }
  } else if (preset.type == e_CameraPreset_LowTouchline) {
    // Aim a few metres infield so the thrower stays in the foreground while
    // the intended receiving area remains visible.
    const float ySide = input.ballPosition.coords[1] < 0.0f ? -1.0f : 1.0f;
    focus = input.ballPosition + Vector3(0.0f, -ySide * 5.5f, 1.1f);
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
  if (preset.type == e_CameraPreset_CosmosWide) {
    // A broadcast rail camera follows the play along the stand, but less than
    // one-for-one. Keeping this residual separation from the ball produces a
    // stable three-quarter view instead of flattening into a perpendicular
    // tactical angle near either penalty area.
    anchor.coords[0] = ClampValue(input.ballPosition.coords[0] * 0.38f,
                                  -pitchHalfW * 0.36f, pitchHalfW * 0.36f);
    const float cameraSide = preset.anchorY < 0.0f ? -1.0f : 1.0f;
    const float nearTouchline = ClampValue(
        (cameraSide * input.ballPosition.coords[1] / pitchHalfH - 0.45f) / 0.50f,
        0.0f, 1.0f);
    const float oppositeTouchline = ClampValue(
        ((-cameraSide * input.ballPosition.coords[1]) / pitchHalfH - 0.35f) / 0.55f,
        0.0f, 1.0f);
    const float goalLineDepth = ClampValue(
        (std::fabs(input.ballPosition.coords[0]) / pitchHalfW - 0.55f) / 0.40f,
        0.0f, 1.0f);
    // Dolly away from the near touchline as the ball approaches it. Increasing
    // the ground separation flattens the viewing angle and removes the
    // top-down look that a fixed high mount produces in its blind zone. The
    // extra goal-line term is essential at the camera-side corner, where the
    // longitudinal rail offset otherwise consumes much of the useful frame.
    anchor.coords[1] += cameraSide * nearTouchline *
        (12.0f + goalLineDepth * 8.0f);
    // On the opposite wing there is no blind zone, so move the mount slightly
    // toward the pitch. Combined with the longer lens below this makes remote
    // players more readable without losing the broadcast diagonal.
    anchor.coords[1] -= cameraSide * oppositeTouchline * 4.5f;
  } else if (preset.type == e_CameraPreset_HighGoalSide || preset.type == e_CameraPreset_LowGoalSide) {
    anchor.coords[0] *= xSide;
    anchor.coords[1] = std::fabs(anchor.coords[1]) * ySide;
  } else if (preset.type == e_CameraPreset_BehindGoal) {
    anchor.coords[0] *= xSide;
  } else if (preset.type == e_CameraPreset_LowTouchline) {
    const float towardMidfield = input.ballPosition.coords[0] < 0.0f ? 10.0f : -10.0f;
    anchor.coords[0] = ClampValue(input.ballPosition.coords[0] + towardMidfield,
                                  -pitchHalfW * 0.82f, pitchHalfW * 0.82f);
    anchor.coords[1] = std::fabs(preset.anchorY * pitchHalfH) * ySide;
  } else if (preset.type == e_CameraPreset_Corner) {
    const Vector3 penaltySpot(input.attackingGoalSide * (pitchHalfW - 11.0f), 0.0f, 0.0f);
    const Vector3 deliveryDirection =
        (input.ballPosition - input.primaryPlayerPosition).Get2D().GetNormalized(
            (penaltySpot - input.ballPosition).Get2D().GetNormalized(
                Vector3(-xSide, -ySide, 0.0f)));
    // Follow the real taker rather than an absolute corner coordinate. This
    // keeps the player in the foreground and the penalty area beyond him.
    // Stand behind the player's run-up line (player -> ball), not behind the
    // eventual cross direction. This keeps the full taker, ball and box in
    // the same wide composition.
    anchor = input.primaryPlayerPosition - deliveryDirection * 3.2f +
             Vector3(0.0f, 0.0f, 4.8f);
  } else if (preset.type == e_CameraPreset_Penalty) {
    const float goalSide = input.attackingGoalSide;
    const Vector3 goal(goalSide * pitchHalfW, 0.0f, 0.0f);
    const Vector3 shotDirection =
        (goal - input.primaryPlayerPosition).Get2D().GetNormalized(
            Vector3(goalSide, 0.0f, 0.0f));
    anchor = input.primaryPlayerPosition - shotDirection * 4.0f +
             Vector3(0.0f, 0.0f, 3.2f);
  } else if (preset.type == e_CameraPreset_FreeKick) {
    const float goalSide = input.attackingGoalSide;
    const Vector3 goal(goalSide * pitchHalfW, 0.0f, 0.0f);
    const Vector3 kickDirection = (goal - input.primaryPlayerPosition).Get2D().GetNormalized(
        Vector3(goalSide, 0.0f, 0.0f));
    anchor = input.primaryPlayerPosition - kickDirection * 5.0f +
             Vector3(0.0f, 0.0f, 3.6f);
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
  const bool goalKickJustTaken = previousSetPieceActive &&
      previousSetPiece == e_SetPiece_GoalKick && !input.setPieceActive;
  if (!initialized) {
    directorStart_ms = input.time_ms;
    clipStart_ms = input.time_ms;
    ActivatePreset(SelectPreset(input), input);
    initialized = true;
    cut = true;
  }

  // The referee buffer stops identifying the corner as soon as the ball is
  // released. Preserve a short delivery phase so the main camera can perform
  // the real broadcast sequence: wide establishing frame, then an assertive
  // zoom following the cross into the penalty area.
  const bool cornerJustTaken = previousSetPiece == e_SetPiece_Corner &&
      input.setPiece != e_SetPiece_Corner;
  if (cornerJustTaken && activePreset == e_CameraPreset_CosmosWide) {
    const float cameraSide = activeAnchor.coords[1] < 0.0f ? -1.0f : 1.0f;
    const bool fromCameraSide =
        cameraSide * input.ballPosition.coords[1] > pitchHalfH * 0.65f;
    if (fromCameraSide) cornerDeliveryUntil_ms = input.time_ms + 1800;
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
    else if (input.setPiece == e_SetPiece_Penalty) requested = e_CameraPreset_Penalty;
    else if (input.setPiece == e_SetPiece_FreeKick) requested = e_CameraPreset_FreeKick;
  }
  if (mode == e_Mode_Procedural && !input.setPieceActive &&
      activePreset == e_CameraPreset_Corner) {
    requested = e_CameraPreset_HighGoalSide;
  }
  if (mode == e_Mode_Procedural && !input.setPieceActive &&
      (activePreset == e_CameraPreset_Penalty ||
       activePreset == e_CameraPreset_FreeKick)) {
    requested = e_CameraPreset_HighGoalSide;
  }
  if (mode == e_Mode_Procedural && !input.setPieceActive &&
      activePreset == e_CameraPreset_LowTouchline) {
    requested = e_CameraPreset_MainBroadcast;
  }
  if (mode == e_Mode_Procedural && !input.inPlay && !input.setPieceActive &&
      activePreset == e_CameraPreset_LowGoalSide) {
    requested = e_CameraPreset_MainBroadcast;
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
  if (mode == e_Mode_Procedural && !input.setPieceActive &&
      activePreset == e_CameraPreset_HighGoalSide &&
      (!finalThirdActive || goalKickJustTaken)) {
    requested = e_CameraPreset_MainBroadcast;
  }
  if (mode == e_Mode_Procedural && activePreset == e_CameraPreset_BehindGoal &&
      (!input.inPlay || input.setPieceActive || !finalThirdActive)) {
    requested = e_CameraPreset_MainBroadcast;
  }
  // A camera-test scenario is authoritative: it deliberately bypasses the
  // editorial selector while keeping the real tracking/framing implementation.
  if (scriptedPreset != e_CameraPreset_Invalid) requested = scriptedPreset;
  if (requested != activePreset) {
    ActivatePreset(requested, input);
    ++cutsInClip;
    cut = true;
  }

  const CameraPreset &preset = presets[activePreset];
  if (activePreset == e_CameraPreset_FullFieldDebug ||
      activePreset == e_CameraPreset_FullFieldPanoramic) {
    // A genuinely static optical history is intentional here: Cosmos sees
    // every player from frame zero, with no pan, zoom, tracking or cuts.
    CameraDirectorOutput output;
    output.position = Vector3(0.0f, preset.anchorY * pitchHalfH,
                              preset.height * pitchHalfH);
    output.yaw = 0.0f;
    // The closer debug view favours the playing surface; the panoramic view
    // recreates the original centred stadium composition from the screenshot.
    const float targetY = activePreset == e_CameraPreset_FullFieldDebug ? -7.0f : 0.0f;
    output.pitch = std::atan2(targetY - output.position.coords[1],
                              output.position.coords[2]);
    output.fov = preset.fov;
    output.nearCap = 1.0f;
    output.farCap = 260.0f;
    output.cut = cut;
    output.preset = activePreset;
    output.cameraName = PresetName(activePreset);
    output.cameraId = cameraId;
    previousSetPieceActive = input.setPieceActive;
    previousSetPiece = input.setPiece;
    return output;
  }
  if (input.setPieceActive &&
      (activePreset == e_CameraPreset_Corner ||
       activePreset == e_CameraPreset_Penalty ||
       activePreset == e_CameraPreset_FreeKick)) {
    // Restart cameras are selected before the referee necessarily places the
    // taker. Rebuild their mount while preparing the kick, so the final frame
    // is tied to the actual taker rather than the player selected at the cut.
    activeAnchor = BuildAnchor(preset, input);
  }
  const Vector3 desiredFocus = BuildFocus(preset, input);
  if (!cut) {
    const float focusFactor = SmoothFactor(preset.trackingResponse, input.delta_ms);
    smoothedFocus = smoothedFocus * (1.0f - focusFactor) + desiredFocus * focusFactor;
  }

  if (!cut && activePreset == e_CameraPreset_CosmosWide) {
    const Vector3 desiredAnchor = BuildAnchor(preset, input);
    // The rail follows more slowly than the pan head, preventing ball jitter
    // and retaining the familiar angled sweep across the pitch.
    const float railFactor = SmoothFactor(0.72f, input.delta_ms);
    const float lateralRailFactor = SmoothFactor(1.35f, input.delta_ms);
    activeAnchor.coords[0] +=
        (desiredAnchor.coords[0] - activeAnchor.coords[0]) * railFactor;
    activeAnchor.coords[1] +=
        (desiredAnchor.coords[1] - activeAnchor.coords[1]) * lateralRailFactor;
  }

  const float finalThird = std::fabs(input.ballPosition.coords[0]) / pitchHalfW;
  // Start the attacking zoom with 30 metres remaining and reach its full
  // strength at 25 metres. The narrow ramp makes the response assertive while
  // interpolation below keeps the lens movement continuous.
  const float finalThirdZoom = ClampValue(
      (std::fabs(input.ballPosition.coords[0]) - (pitchHalfW - 30.0f)) / 5.0f,
      0.0f, 1.0f);
  const float speed = ClampValue(input.ballMovement.GetLength() / 18.0f, 0.0f, 1.0f);
  // Dataset observation: the main camera stays comparatively wide during a
  // fast transition, then tightens once possession settles around the box.
  // This term creates that extra operator zoom without tying it to a cut.
  const float settledAttackZoom = finalThirdZoom * (1.0f - speed);
  // The far touchline needs a little more lens than the near side to keep
  // players equally legible. Derive it from the physical side of the active
  // mount, independently from progress toward either goal.
  const float cameraSide = activeAnchor.coords[1] < 0.0f ? -1.0f : 1.0f;
  const float oppositeTouchlineZoom = ClampValue(
      ((-cameraSide * input.ballPosition.coords[1]) / pitchHalfH - 0.05f) / 0.85f,
      0.0f, 1.0f);
  const float ballGroundDistance = (input.ballPosition - activeAnchor).Get2D().GetLength();
  const float nearCamera = ClampValue((28.0f - ballGroundDistance) / 22.0f, 0.0f, 1.0f);
  // The far-side tightening is deliberately modest for COSMOS_WIDE: real
  // master shots still preserve tactical context around the ball.
  const float farCamera = ClampValue((ballGroundDistance - 48.0f) / 28.0f, 0.0f, 1.0f);
  float desiredFov = preset.fov + speed * 2.5f - finalThird * 3.0f;
  bool sameSideCornerDelivery = false;
  if (activePreset == e_CameraPreset_CosmosWide) {
    desiredFov -= farCamera * 3.5f;
    desiredFov -= finalThirdZoom * 7.5f;
    desiredFov -= settledAttackZoom * 2.5f;
    desiredFov -= oppositeTouchlineZoom * 5.5f;
    if (input.setPieceActive && finalThirdZoom > 0.0f) {
      desiredFov -= 1.0f;
    }
    const bool sameSideCorner = input.setPieceActive &&
        input.setPiece == e_SetPiece_Corner &&
        input.ballPosition.coords[1] * activeAnchor.coords[1] > 0.0f;
    if (sameSideCorner) {
      // Establishing phase: show the taker and almost the entire penalty area.
      desiredFov = std::max(desiredFov, preset.maxFov);
    }
    sameSideCornerDelivery = input.time_ms < cornerDeliveryUntil_ms;
    if (sameSideCornerDelivery) {
      // Delivery phase: close decisively on the moving ball and the group in
      // the box, matching the fast lens pull used immediately after the kick.
      desiredFov = 20.5f;
    }
  }
  if (activePreset == e_CameraPreset_HighGoalSide) {
    const float nearTouchline = std::fabs(input.ballPosition.coords[1]) / pitchHalfH;
    desiredFov = preset.fov + speed * 1.5f + nearTouchline * 5.0f;
  }
  // Widen every preset as play enters the patch directly below its physical
  // mount. This is the common blind zone of sideline and diagonal cameras.
  desiredFov += nearCamera * 8.0f;
  desiredFov = ClampValue(desiredFov, preset.minFov, preset.maxFov);
  if (!cut) {
    const float zoomResponse = sameSideCornerDelivery ? 4.8f : preset.zoomResponse;
    const float zoomFactor = SmoothFactor(zoomResponse, input.delta_ms);
    smoothedFov += (desiredFov - smoothedFov) * zoomFactor;
  }

  // When the ball is close to the camera, editorial lead-room must yield to
  // ball retention. Blending here affects framing only; normal tracking
  // resumes smoothly as the play moves away.
  // COSMOS_WIDE can physically dolly away from its near-side blind zone, so
  // it needs much less emergency ball centring. Retaining the composed focus
  // is essential for showing both the winger/corner taker and central players.
  const float nearTouchlineRetention = activePreset == e_CameraPreset_CosmosWide
      ? ClampValue(
          (cameraSide * input.ballPosition.coords[1] / pitchHalfH - 0.45f) / 0.50f,
          0.0f, 1.0f) * 0.22f
      : 0.0f;
  const float nearBallRetention = std::max(
      nearCamera * (activePreset == e_CameraPreset_CosmosWide ? 0.32f : 0.88f),
      nearTouchlineRetention);
  Vector3 framingFocus = smoothedFocus * (1.0f - nearBallRetention) +
                         input.ballPosition * nearBallRetention;
  framingFocus.coords[2] = smoothedFocus.coords[2];
  const Vector3 toTarget = framingFocus - activeAnchor;
  const float centerYaw = (Vector3(0, 0, 0) - activeAnchor).GetAngle2D() + 1.5f * kPi;
  float yaw = toTarget.GetAngle2D() + 1.5f * kPi;
  if (activePreset != e_CameraPreset_CosmosWide) {
    const float maxPan = preset.maxPanDegrees * kPi / 180.0f;
    yaw = centerYaw + ClampValue(WrapAngle(yaw - centerYaw), -maxPan, maxPan);
  }
  // COSMOS_WIDE deliberately uses the unconstrained target yaw. Its rail,
  // outward dolly and infield focus determine the broadcast composition; a
  // centre-referenced clamp is geometrically invalid at the near corner and
  // was the remaining source of the apparent locked axis.
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
  previousSetPieceActive = input.setPieceActive;
  previousSetPiece = input.setPiece;
  return output;
}

std::string CameraDirector::GetActiveCameraName() const {
  return PresetName(activePreset);
}
