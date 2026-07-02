#ifndef _HPP_SOCCERREPLAY_EXPORTER
#define _HPP_SOCCERREPLAY_EXPORTER

#include "soccerreplay_event.hpp"

#include <set>
#include <string>
#include <vector>

class MatchData;
class PlayerData;
class TeamData;

namespace dataset {
namespace soccerreplay1988 {

struct MatchInformation {
  std::string timestamp;
  std::string score;
  std::string home_team;
  std::string away_team;
  std::string home_formation;
  std::string away_formation;
  std::string venue;
  std::string capacity;
  std::string attendance;
};

struct RefereeInformation {
  std::string country;
  std::string name;
};

struct PlayerInformation {
  std::string players_name;
  std::string players_number;
  std::string full_name;
  std::string players_rating;
  std::string country;
  std::string role;
  std::string age_and_birthdate;
  std::string market_value;
};

struct FrameDumpRecord {
  int frame_number;
  int half;
  std::string time_stamp;
  std::string file;
  unsigned long match_time_ms;
  unsigned long actual_time_ms;
};

class SoccerReplayExporter {
  public:
    SoccerReplayExporter(MatchData *matchData, const std::string &outputRoot);

    bool IsEnabled() const { return enabled; }
    const std::string &GetOutputDirectory() const { return outputDirectory; }
    const std::string &GetFramesDirectory() const { return framesDirectory; }
    const std::string &GetClipsDirectory() const { return clipsDirectory; }

    void SetScore(int homeGoals, int awayGoals);
    bool RecordEvent(const EventDescription &eventDescription);
    bool RecordHalfEndOnce(const std::string &phaseKey, const EventDescription &eventDescription);
    std::string RegisterFrameDump(int half, const std::string &timeStamp, unsigned long matchTime_ms, unsigned long actualTime_ms);
    bool FlushAnnotationsOnly();
    bool Flush();

  private:
    void BuildInitialMetadata(MatchData *matchData);
    void AddTeamPlayers(const TeamData *teamData);
    std::string BuildFormationString(const TeamData *teamData) const;
    std::string BuildShortPlayerName(const PlayerData *playerData) const;
    bool FlushAnnotations();
    bool FlushFrameIndex();
    std::string BuildFrameFilename(int frameNumber) const;

    MatchInformation matchInformation;
    RefereeInformation refereeInformation;
    std::vector<PlayerInformation> playerInformation;
    std::vector<EventDescription> eventDescriptions;
    std::vector<FrameDumpRecord> frameDumpRecords;
    std::set<std::string> closedPhaseKeys;

    std::string outputDirectory;
    std::string framesDirectory;
    std::string clipsDirectory;
    std::string annotationsFilename;
    std::string frameIndexFilename;
    bool enabled;
};

}
}

#endif
