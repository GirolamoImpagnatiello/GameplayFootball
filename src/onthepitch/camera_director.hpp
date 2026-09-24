#ifndef _HPP_CAMERA_DIRECTOR
#define _HPP_CAMERA_DIRECTOR

#include "base/math/vector3.hpp"
#include "base/properties.hpp"
#include "../gamedefines.hpp"

#include <map>
#include <string>
#include <vector>

enum e_CameraPresetType {
  e_CameraPreset_MainBroadcast,
  e_CameraPreset_HighWide,
  e_CameraPreset_HighGoalSide,
  e_CameraPreset_LowTouchline,
  e_CameraPreset_LowGoalSide,
  e_CameraPreset_Corner,
  e_CameraPreset_BehindGoal,
  e_CameraPreset_CloseUp,
  e_CameraPreset_Invalid
};

enum e_CameraTrackingMode {
  e_CameraTracking_Action,
  e_CameraTracking_Tactical,
  e_CameraTracking_GoalArea,
  e_CameraTracking_Player
};

struct CameraPreset {
  e_CameraPresetType type;
  std::string name;
  float anchorX;
  float anchorY;
  float height;
  float fov;
  float minFov;
  float maxFov;
  float maxPanDegrees;
  float minTiltDegrees;
  float maxTiltDegrees;
  float trackingResponse;
  float zoomResponse;
  float weight;
  float minShotSeconds;
  float maxShotSeconds;
  e_CameraTrackingMode tracking;
};

struct CameraDirectorInput {
  unsigned long time_ms;
  unsigned long delta_ms;
  Vector3 ballPosition;
  Vector3 ballMovement;
  Vector3 nearbyPlayersCenter;
  unsigned int nearbyPlayerCount;
  Vector3 primaryPlayerPosition;
  Vector3 actionDirection;
  bool inPlay;
  bool setPieceActive;
  e_SetPiece setPiece;
  unsigned long setPieceStartTime_ms;
};

struct CameraDirectorOutput {
  Vector3 position;
  float yaw;
  float pitch;
  float fov;
  float nearCap;
  float farCap;
  bool cut;
  e_CameraPresetType preset;
  std::string cameraName;
  unsigned int cameraId;
};

class CameraDirector {
 public:
  explicit CameraDirector(Properties *configuration);

  bool IsEnabled() const { return enabled; }
  CameraDirectorOutput Update(const CameraDirectorInput &input);
  std::string GetActiveCameraName() const;
  unsigned int GetActiveCameraId() const { return cameraId; }

  static e_CameraPresetType ParsePresetType(const std::string &name);
  static const char *PresetName(e_CameraPresetType type);

 private:
  struct ScheduleEntry {
    unsigned long start_ms;
    e_CameraPresetType preset;
  };

  enum Mode {
    e_Mode_Legacy,
    e_Mode_Single,
    e_Mode_Scheduled,
    e_Mode_Procedural
  };

  void BuildPresets(Properties *configuration);
  void ParseSchedule(const std::string &value);
  e_CameraPresetType SelectPreset(const CameraDirectorInput &input);
  void ActivatePreset(e_CameraPresetType type, const CameraDirectorInput &input);
  float Random01();
  float RandomRange(float minimum, float maximum);
  float GetContextWeight(e_CameraPresetType type, const CameraDirectorInput &input) const;
  Vector3 BuildFocus(const CameraPreset &preset, const CameraDirectorInput &input) const;
  Vector3 BuildAnchor(const CameraPreset &preset, const CameraDirectorInput &input) const;

  Mode mode;
  bool enabled;
  bool closeUpEnabled;
  bool initialized;
  bool finalThirdActive;
  unsigned int randomState;
  unsigned int cameraId;
  int maxCutsPerClip;
  int cutsInClip;
  unsigned long clipDuration_ms;
  unsigned long directorStart_ms;
  unsigned long clipStart_ms;
  unsigned long shotStart_ms;
  unsigned long shotEnd_ms;
  e_CameraPresetType singlePreset;
  e_CameraPresetType activePreset;
  Vector3 activeAnchor;
  Vector3 smoothedFocus;
  float smoothedFov;
  std::map<e_CameraPresetType, CameraPreset> presets;
  std::vector<ScheduleEntry> schedule;
};

#endif
