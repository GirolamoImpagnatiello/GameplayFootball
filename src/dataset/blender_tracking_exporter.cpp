#include "blender_tracking_exporter.hpp"

#include <boost/filesystem.hpp>

#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace dataset {
namespace blendertracking {

namespace {

static std::string JsonEscape(const std::string &value) {
  std::ostringstream result;
  for (std::string::const_iterator iter = value.begin(); iter != value.end(); ++iter) {
    const char c = *iter;
    switch (c) {
      case '\\': result << "\\\\"; break;
      case '"': result << "\\\""; break;
      case '\b': result << "\\b"; break;
      case '\f': result << "\\f"; break;
      case '\n': result << "\\n"; break;
      case '\r': result << "\\r"; break;
      case '\t': result << "\\t"; break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          result << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
        } else {
          result << c;
        }
        break;
    }
  }
  return result.str();
}

static std::string Quote(const std::string &value) {
  return "\"" + JsonEscape(value) + "\"";
}

static std::string FormatNow(const char *format) {
  std::time_t now = std::time(0);
  std::tm localNow;
#ifdef _WIN32
  localtime_s(&localNow, &now);
#else
  localtime_r(&now, &localNow);
#endif
  char buffer[64];
  std::strftime(buffer, sizeof(buffer), format, &localNow);
  return std::string(buffer);
}

static void WriteVec3(std::ofstream &output, const Vec3 &value) {
  output << "\"x\": " << value.x << ", \"y\": " << value.y << ", \"z\": " << value.z;
}

}

BlenderTrackingExporter::BlenderTrackingExporter(const std::string &outputRoot, int fps)
  : fps(fps), writtenFrames(0), lastFrame(1), enabled(false), finalized(false) {

  if (this->fps < 1) this->fps = 1;
  if (this->fps > 120) this->fps = 120;

  const std::string safeOutputRoot = outputRoot.empty() ? "output/blender_tracking" : outputRoot;
  outputDirectory = (boost::filesystem::path(safeOutputRoot) / ("match_" + FormatNow("%Y%m%d_%H%M%S"))).string();
  jsonFilename = (boost::filesystem::path(outputDirectory) / "tracking_blender.json").string();

  try {
    boost::filesystem::create_directories(outputDirectory);
    enabled = WriteHeader();
  } catch (...) {
    enabled = false;
  }
}

BlenderTrackingExporter::~BlenderTrackingExporter() {
  Flush();
}

bool BlenderTrackingExporter::WriteHeader() {
  std::ofstream output(jsonFilename.c_str(), std::ios::out | std::ios::trunc);
  if (!output.is_open()) return false;

  output << "{\n";
  output << "  \"schema_version\": 1,\n";
  output << "  \"name\": \"gameplayfootball_blender_tracking\",\n";
  output << "  \"fps\": " << fps << ",\n";
  output << "  \"frame_start\": 1,\n";
  output << "  \"coordinate_space\": \"blender_meters\",\n";
  output << "  \"model_yaw_offset_degrees\": 90,\n";
  output << "  \"default_actions\": {\n";
  output << "    \"A\": [\"Offensive Idle\", \"Idle\"],\n";
  output << "    \"B\": [\"Idle\", \"Offensive Idle\"]\n";
  output << "  },\n";
  output << "  \"frames\": [\n";
  return true;
}

bool BlenderTrackingExporter::RecordFrame(const FrameSnapshot &snapshot) {
  if (!enabled || finalized) return false;
  WriteFrame(snapshot);
  writtenFrames++;
  if (snapshot.frame > lastFrame) lastFrame = snapshot.frame;
  return true;
}

void BlenderTrackingExporter::WriteFrame(const FrameSnapshot &snapshot) {
  std::ofstream output(jsonFilename.c_str(), std::ios::out | std::ios::app);
  if (!output.is_open()) {
    enabled = false;
    return;
  }

  if (writtenFrames > 0) output << ",\n";

  output << "    {\n";
  output << "      \"frame\": " << snapshot.frame << ",\n";
  output << "      \"half\": " << snapshot.half << ",\n";
  output << "      \"match_time_ms\": " << snapshot.match_time_ms << ",\n";
  output << "      \"actual_time_ms\": " << snapshot.actual_time_ms << ",\n";
  output << "      \"time_stamp\": " << Quote(snapshot.time_stamp) << ",\n";
  output << "      \"ball\": {";
  WriteVec3(output, snapshot.ball.position);
  output << ", \"velocity\": {";
  WriteVec3(output, snapshot.ball.velocity);
  output << "}},\n";
  output << "      \"players\": {\n";

  for (std::size_t i = 0; i < snapshot.players.size(); ++i) {
    const PlayerSnapshot &player = snapshot.players[i];
    output << "        " << Quote(player.id) << ": {";
    output << "\"x\": " << player.position.x << ", ";
    output << "\"y\": " << player.position.y << ", ";
    output << "\"z\": " << player.position.z << ", ";
    output << "\"yaw_degrees\": " << player.yaw_degrees << ", ";
    output << "\"body_yaw_degrees\": " << player.body_yaw_degrees << ", ";
    output << "\"movement\": {";
    WriteVec3(output, player.movement);
    output << "}, ";
    output << "\"velocity\": " << player.velocity << ", ";
    output << "\"velocity_type\": " << Quote(player.velocity_type) << ", ";
    output << "\"function_type\": " << Quote(player.function_type) << ", ";
    output << "\"sim_anim\": " << Quote(player.sim_anim) << ", ";
    output << "\"anim_frame\": " << player.anim_frame << ", ";
    output << "\"anim_frame_count\": " << player.anim_frame_count << ", ";
    output << "\"touch_frame\": " << player.touch_frame << ", ";
    output << "\"blender_action\": " << Quote(player.blender_action) << ", ";
    output << "\"role\": " << Quote(player.role) << ", ";
    output << "\"team_id\": " << player.team_id << ", ";
    output << "\"player_id\": " << player.player_id << ", ";
    output << "\"sim_id\": " << Quote(player.sim_id) << ", ";
    output << "\"has_possession\": " << (player.has_possession ? "true" : "false") << ", ";
    output << "\"is_ball_retainer\": " << (player.is_ball_retainer ? "true" : "false");
    output << "}" << (i + 1 == snapshot.players.size() ? "\n" : ",\n");
  }

  output << "      }\n";
  output << "    }";
}

void BlenderTrackingExporter::WriteFooter() {
  std::ofstream output(jsonFilename.c_str(), std::ios::out | std::ios::app);
  if (!output.is_open()) {
    enabled = false;
    return;
  }

  const int frameEnd = writtenFrames > 0 ? lastFrame : 1;
  output << "\n";
  output << "  ],\n";
  output << "  \"frame_end\": " << frameEnd << ",\n";
  output << "  \"action_segments\": []\n";
  output << "}\n";
}

bool BlenderTrackingExporter::Flush() {
  if (!enabled || finalized) return enabled;
  WriteFooter();
  finalized = true;
  return enabled;
}

}
}
