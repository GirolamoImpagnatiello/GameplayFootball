#include "soccerreplay_exporter.hpp"

#include "soccerreplay_labels.hpp"

#include "data/matchdata.hpp"
#include "data/playerdata.hpp"
#include "data/teamdata.hpp"

#include "gamedefines.hpp"

#include <boost/filesystem.hpp>

#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace dataset {
namespace soccerreplay1988 {

const std::vector<std::string> &GetCanonicalLabels() {
  static const std::vector<std::string> canonicalLabels = {
    labels::Corner,
    labels::Goal,
    labels::Injury,
    labels::OwnGoal,
    labels::Penalty,
    labels::PenaltyMissed,
    labels::RedCard,
    labels::SecondYellowCard,
    labels::Substitution,
    labels::StartOfGameHalf,
    labels::EndOfGameHalf,
    labels::YellowCard,
    labels::ThrowIn,
    labels::FreeKick,
    labels::SavedByGoalKeeper,
    labels::ShotOffTarget,
    labels::Clearance,
    labels::LeadToCorner,
    labels::OffSide,
    labels::Var,
    labels::FoulNoCard,
    labels::StatisticsAndSummary,
    labels::BallPossession,
    labels::BallOutOfPlay
  };
  return canonicalLabels;
}

bool IsCanonicalLabel(const std::string &label) {
  const std::vector<std::string> &canonicalLabels = GetCanonicalLabels();
  for (std::vector<std::string>::const_iterator iter = canonicalLabels.begin(); iter != canonicalLabels.end(); ++iter) {
    if (*iter == label) return true;
  }
  return false;
}

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

SoccerReplayExporter::SoccerReplayExporter(MatchData *matchData, const std::string &outputRoot) : enabled(false) {
  std::string safeOutputRoot = outputRoot.empty() ? "output/datasets/soccerreplay1988" : outputRoot;
  outputDirectory = (boost::filesystem::path(safeOutputRoot) / ("match_" + FormatNow("%Y%m%d_%H%M%S"))).string();
  framesDirectory = (boost::filesystem::path(outputDirectory) / "frames").string();
  annotationsFilename = (boost::filesystem::path(outputDirectory) / "annotations.json").string();
  frameIndexFilename = (boost::filesystem::path(outputDirectory) / "frame_index.json").string();

  try {
    boost::filesystem::create_directories(outputDirectory);
    boost::filesystem::create_directories(framesDirectory);
    enabled = true;
  } catch (...) {
    enabled = false;
  }

  BuildInitialMetadata(matchData);
}

void SoccerReplayExporter::BuildInitialMetadata(MatchData *matchData) {
  matchInformation.timestamp = FormatNow("%Y-%m-%d %H:%M:%S");
  matchInformation.score = "0 - 0";
  matchInformation.venue = "";
  matchInformation.capacity = "";
  matchInformation.attendance = "";

  if (matchData) {
    TeamData *homeTeamData = matchData->GetTeamData(0);
    TeamData *awayTeamData = matchData->GetTeamData(1);
    if (homeTeamData) {
      matchInformation.home_team = homeTeamData->GetName();
      matchInformation.home_formation = BuildFormationString(homeTeamData);
      AddTeamPlayers(homeTeamData);
    }
    if (awayTeamData) {
      matchInformation.away_team = awayTeamData->GetName();
      matchInformation.away_formation = BuildFormationString(awayTeamData);
      AddTeamPlayers(awayTeamData);
    }
  }

  refereeInformation.country = "";
  refereeInformation.name = "";
}

void SoccerReplayExporter::SetScore(int homeGoals, int awayGoals) {
  std::ostringstream score;
  score << homeGoals << " - " << awayGoals;
  matchInformation.score = score.str();
}

bool SoccerReplayExporter::RecordEvent(const EventDescription &eventDescription) {
  if (!enabled) return false;
  if (!IsCanonicalLabel(eventDescription.comments_type)) return false;
  eventDescriptions.push_back(eventDescription);
  return true;
}

bool SoccerReplayExporter::RecordHalfEndOnce(const std::string &phaseKey, const EventDescription &eventDescription) {
  if (closedPhaseKeys.find(phaseKey) != closedPhaseKeys.end()) return false;
  closedPhaseKeys.insert(phaseKey);
  return RecordEvent(eventDescription);
}

std::string SoccerReplayExporter::BuildFrameFilename(int frameNumber) const {
  std::ostringstream filename;
  filename << "frame_" << std::setfill('0') << std::setw(6) << frameNumber << ".png";
  return filename.str();
}

std::string SoccerReplayExporter::RegisterFrameDump(int half, const std::string &timeStamp, unsigned long matchTime_ms, unsigned long actualTime_ms) {
  if (!enabled) return "";

  FrameDumpRecord frameDumpRecord;
  frameDumpRecord.frame_number = static_cast<int>(frameDumpRecords.size()) + 1;
  frameDumpRecord.half = half;
  frameDumpRecord.time_stamp = timeStamp;
  frameDumpRecord.file = "frames/" + BuildFrameFilename(frameDumpRecord.frame_number);
  frameDumpRecord.match_time_ms = matchTime_ms;
  frameDumpRecord.actual_time_ms = actualTime_ms;
  frameDumpRecords.push_back(frameDumpRecord);

  return (boost::filesystem::path(outputDirectory) / frameDumpRecord.file).string();
}

bool SoccerReplayExporter::Flush() {
  if (!enabled) return false;
  const bool annotationsOk = FlushAnnotations();
  const bool frameIndexOk = FlushFrameIndex();
  return annotationsOk && frameIndexOk;
}

bool SoccerReplayExporter::FlushAnnotations() {
  if (!enabled) return false;

  std::ofstream output(annotationsFilename.c_str(), std::ios::out | std::ios::trunc);
  if (!output.is_open()) return false;

  output << "{\n";
  output << "  \"match_information\": {\n";
  output << "    \"timestamp\": " << Quote(matchInformation.timestamp) << ",\n";
  output << "    \"score\": " << Quote(matchInformation.score) << ",\n";
  output << "    \"home_team\": " << Quote(matchInformation.home_team) << ",\n";
  output << "    \"away_team\": " << Quote(matchInformation.away_team) << ",\n";
  output << "    \"home_formation\": " << Quote(matchInformation.home_formation) << ",\n";
  output << "    \"away_formation\": " << Quote(matchInformation.away_formation) << ",\n";
  output << "    \"venue\": " << Quote(matchInformation.venue) << ",\n";
  output << "    \"capacity\": " << Quote(matchInformation.capacity) << ",\n";
  output << "    \"attendance\": " << Quote(matchInformation.attendance) << "\n";
  output << "  },\n";

  output << "  \"referee_information\": {\n";
  output << "    \"country\": " << Quote(refereeInformation.country) << ",\n";
  output << "    \"name\": " << Quote(refereeInformation.name) << "\n";
  output << "  },\n";

  output << "  \"player_information\": [\n";
  for (std::size_t i = 0; i < playerInformation.size(); ++i) {
    const PlayerInformation &player = playerInformation[i];
    output << "    {\n";
    output << "      \"players_name\": " << Quote(player.players_name) << ",\n";
    output << "      \"players_number\": " << Quote(player.players_number) << ",\n";
    output << "      \"Full Name\": " << Quote(player.full_name) << ",\n";
    output << "      \"players_rating\": " << Quote(player.players_rating) << ",\n";
    output << "      \"Country\": " << Quote(player.country) << ",\n";
    output << "      \"Role\": " << Quote(player.role) << ",\n";
    output << "      \"Age and Birthdate\": " << Quote(player.age_and_birthdate) << ",\n";
    output << "      \"Market Value\": " << Quote(player.market_value) << "\n";
    output << "    }" << (i + 1 == playerInformation.size() ? "\n" : ",\n");
  }
  output << "  ],\n";

  output << "  \"event_descriptions\": [\n";
  for (std::size_t i = 0; i < eventDescriptions.size(); ++i) {
    const EventDescription &eventDescription = eventDescriptions[i];
    output << "    {\n";
    output << "      \"half\": " << eventDescription.half << ",\n";
    output << "      \"time_stamp\": " << Quote(eventDescription.time_stamp) << ",\n";
    output << "      \"comments_type\": " << Quote(eventDescription.comments_type) << ",\n";
    output << "      \"comments_text\": " << Quote(eventDescription.comments_text) << ",\n";
    output << "      \"comments_text_anonymized\": " << Quote(eventDescription.comments_text_anonymized) << "\n";
    output << "    }" << (i + 1 == eventDescriptions.size() ? "\n" : ",\n");
  }
  output << "  ]\n";
  output << "}\n";

  return true;
}

bool SoccerReplayExporter::FlushFrameIndex() {
  if (!enabled) return false;

  std::ofstream output(frameIndexFilename.c_str(), std::ios::out | std::ios::trunc);
  if (!output.is_open()) return false;

  output << "{\n";
  output << "  \"fps\": 1,\n";
  output << "  \"window_duration_seconds\": 30,\n";
  output << "  \"frame_directory\": \"frames\",\n";
  output << "  \"frames\": [\n";
  for (std::size_t i = 0; i < frameDumpRecords.size(); ++i) {
    const FrameDumpRecord &frame = frameDumpRecords[i];
    output << "    {\n";
    output << "      \"frame_number\": " << frame.frame_number << ",\n";
    output << "      \"file\": " << Quote(frame.file) << ",\n";
    output << "      \"half\": " << frame.half << ",\n";
    output << "      \"time_stamp\": " << Quote(frame.time_stamp) << ",\n";
    output << "      \"match_time_ms\": " << frame.match_time_ms << ",\n";
    output << "      \"actual_time_ms\": " << frame.actual_time_ms << "\n";
    output << "    }" << (i + 1 == frameDumpRecords.size() ? "\n" : ",\n");
  }
  output << "  ],\n";

  output << "  \"events\": [\n";
  for (std::size_t eventIndex = 0; eventIndex < eventDescriptions.size(); ++eventIndex) {
    const EventDescription &eventDescription = eventDescriptions[eventIndex];
    std::vector<std::size_t> halfFrameIndexes;
    for (std::size_t frameIndex = 0; frameIndex < frameDumpRecords.size(); ++frameIndex) {
      if (frameDumpRecords[frameIndex].half == eventDescription.half) halfFrameIndexes.push_back(frameIndex);
    }

    int frameCenter = 0;
    int frameStart = 0;
    int frameEnd = 0;
    int frameCount = 0;
    unsigned long windowStart_ms = eventDescription.match_time_ms;
    unsigned long windowEnd_ms = eventDescription.match_time_ms;
    bool eventInsideWindow = false;

    if (!halfFrameIndexes.empty()) {
      std::size_t closestHalfFrame = 0;
      unsigned long closestDistance = static_cast<unsigned long>(-1);
      for (std::size_t i = 0; i < halfFrameIndexes.size(); ++i) {
        const FrameDumpRecord &frame = frameDumpRecords[halfFrameIndexes[i]];
        const unsigned long distance = frame.match_time_ms > eventDescription.match_time_ms ?
          frame.match_time_ms - eventDescription.match_time_ms :
          eventDescription.match_time_ms - frame.match_time_ms;
        if (distance < closestDistance) {
          closestDistance = distance;
          closestHalfFrame = i;
        }
      }

      int startHalfFrame = static_cast<int>(closestHalfFrame) - 15;
      if (startHalfFrame < 0) startHalfFrame = 0;
      if (startHalfFrame + 29 >= static_cast<int>(halfFrameIndexes.size())) {
        startHalfFrame = static_cast<int>(halfFrameIndexes.size()) - 30;
        if (startHalfFrame < 0) startHalfFrame = 0;
      }
      int endHalfFrame = startHalfFrame + 29;
      if (endHalfFrame >= static_cast<int>(halfFrameIndexes.size())) endHalfFrame = static_cast<int>(halfFrameIndexes.size()) - 1;

      const FrameDumpRecord &centerFrame = frameDumpRecords[halfFrameIndexes[closestHalfFrame]];
      const FrameDumpRecord &startFrame = frameDumpRecords[halfFrameIndexes[startHalfFrame]];
      const FrameDumpRecord &endFrame = frameDumpRecords[halfFrameIndexes[endHalfFrame]];
      frameCenter = centerFrame.frame_number;
      frameStart = startFrame.frame_number;
      frameEnd = endFrame.frame_number;
      frameCount = frameEnd - frameStart + 1;
      windowStart_ms = startFrame.match_time_ms;
      windowEnd_ms = endFrame.match_time_ms;
      eventInsideWindow = eventDescription.match_time_ms >= windowStart_ms && eventDescription.match_time_ms <= windowEnd_ms;
    }

    output << "    {\n";
    output << "      \"event_id\": " << eventIndex << ",\n";
    output << "      \"half\": " << eventDescription.half << ",\n";
    output << "      \"comments_type\": " << Quote(eventDescription.comments_type) << ",\n";
    output << "      \"time_stamp\": " << Quote(eventDescription.time_stamp) << ",\n";
    output << "      \"event_match_time_ms\": " << eventDescription.match_time_ms << ",\n";
    output << "      \"fps\": 1,\n";
    output << "      \"window_duration_seconds\": 30,\n";
    output << "      \"frame_center\": " << frameCenter << ",\n";
    output << "      \"frame_start\": " << frameStart << ",\n";
    output << "      \"frame_end\": " << frameEnd << ",\n";
    output << "      \"frame_count\": " << frameCount << ",\n";
    output << "      \"window_start_match_time_ms\": " << windowStart_ms << ",\n";
    output << "      \"window_end_match_time_ms\": " << windowEnd_ms << ",\n";
    output << "      \"event_inside_window\": " << (eventInsideWindow ? "true" : "false") << "\n";
    output << "    }" << (eventIndex + 1 == eventDescriptions.size() ? "\n" : ",\n");
  }
  output << "  ]\n";
  output << "}\n";

  return true;
}

void SoccerReplayExporter::AddTeamPlayers(const TeamData *teamData) {
  if (!teamData) return;

  const std::vector<PlayerData*> &players = const_cast<TeamData*>(teamData)->GetPlayerData();
  for (std::size_t i = 0; i < players.size(); ++i) {
    PlayerData *playerData = players[i];
    if (!playerData) continue;

    PlayerInformation player;
    player.players_name = BuildShortPlayerName(playerData);
    std::ostringstream playerNumber;
    playerNumber << (i + 1);
    player.players_number = playerNumber.str();
    player.full_name = playerData->GetFirstName() + " " + playerData->GetLastName();
    player.players_rating = "";
    player.country = "";
    player.role = GetRoleName(const_cast<TeamData*>(teamData)->GetFormationEntry(static_cast<int>(i)).role);
    player.age_and_birthdate = "";
    player.market_value = "";
    playerInformation.push_back(player);
  }
}

std::string SoccerReplayExporter::BuildFormationString(const TeamData *teamData) const {
  if (!teamData) return "";

  int defenders = 0;
  int midfielders = 0;
  int forwards = 0;
  const int playerCount = const_cast<TeamData*>(teamData)->GetPlayerNum();
  for (int i = 0; i < playerCount; ++i) {
    const e_PlayerRole role = const_cast<TeamData*>(teamData)->GetFormationEntry(i).role;
    if (role == e_PlayerRole_CB || role == e_PlayerRole_LB || role == e_PlayerRole_RB) defenders++;
    else if (role == e_PlayerRole_DM || role == e_PlayerRole_CM || role == e_PlayerRole_LM || role == e_PlayerRole_RM || role == e_PlayerRole_AM) midfielders++;
    else if (role == e_PlayerRole_CF) forwards++;
  }

  if (defenders == 0 && midfielders == 0 && forwards == 0) return "";

  std::ostringstream formation;
  formation << defenders << " - " << midfielders << " - " << forwards;
  return formation.str();
}

std::string SoccerReplayExporter::BuildShortPlayerName(const PlayerData *playerData) const {
  if (!playerData) return "";

  std::string result = playerData->GetLastName();
  const std::string firstName = playerData->GetFirstName();
  if (!firstName.empty()) {
    result += " ";
    result += firstName.substr(0, 1);
    result += ".";
  }
  return result;
}

}
}
