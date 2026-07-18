#ifndef _HPP_BLENDER_TRACKING_EXPORTER
#define _HPP_BLENDER_TRACKING_EXPORTER

#include <string>
#include <vector>

namespace dataset {
namespace blendertracking {

struct Vec3 {
  Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
  Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
  float x;
  float y;
  float z;
};

struct BallSnapshot {
  Vec3 position;
  Vec3 velocity;
};

struct PlayerSnapshot {
  std::string id;
  std::string sim_id;
  int team_id;
  int player_id;
  std::string role;
  Vec3 position;
  Vec3 movement;
  float yaw_degrees;
  float body_yaw_degrees;
  float velocity;
  std::string velocity_type;
  std::string function_type;
  std::string sim_anim;
  int anim_frame;
  int anim_frame_count;
  int touch_frame;
  std::string blender_action;
  bool has_possession;
  bool is_ball_retainer;
};

struct FrameSnapshot {
  int frame;
  int half;
  unsigned long match_time_ms;
  unsigned long actual_time_ms;
  std::string time_stamp;
  BallSnapshot ball;
  std::vector<PlayerSnapshot> players;
};

class BlenderTrackingExporter {
  public:
    BlenderTrackingExporter(const std::string &outputRoot, int fps);
    ~BlenderTrackingExporter();

    bool IsEnabled() const { return enabled; }
    const std::string &GetOutputDirectory() const { return outputDirectory; }
    const std::string &GetJsonFilename() const { return jsonFilename; }
    int GetFps() const { return fps; }

    bool RecordFrame(const FrameSnapshot &snapshot);
    bool Flush();

  private:
    bool WriteHeader();
    void WriteFrame(const FrameSnapshot &snapshot);
    void WriteFooter();

    std::string outputDirectory;
    std::string jsonFilename;
    int fps;
    int writtenFrames;
    int lastFrame;
    bool enabled;
    bool finalized;
};

}
}

#endif
