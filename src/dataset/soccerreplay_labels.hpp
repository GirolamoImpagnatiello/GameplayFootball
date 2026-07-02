// SoccerReplay-1988 event labels. Keep these strings aligned with the paper.
#ifndef _HPP_SOCCERREPLAY_LABELS
#define _HPP_SOCCERREPLAY_LABELS

#include <string>
#include <vector>

namespace dataset {
namespace soccerreplay1988 {

namespace labels {
  static const char *const Corner = "corner";
  static const char *const Goal = "goal";
  static const char *const Injury = "injury";
  static const char *const OwnGoal = "own goal";
  static const char *const Penalty = "penalty";
  static const char *const PenaltyMissed = "penalty missed";
  static const char *const RedCard = "red card";
  static const char *const SecondYellowCard = "second yellow card";
  static const char *const Substitution = "substitution";
  static const char *const StartOfGameHalf = "start of game(half)";
  static const char *const EndOfGameHalf = "end of game(half)";
  static const char *const YellowCard = "yellow card";
  static const char *const ThrowIn = "throw in";
  static const char *const FreeKick = "free kick";
  static const char *const SavedByGoalKeeper = "saved by goal-keeper";
  static const char *const ShotOffTarget = "shot off target";
  static const char *const Clearance = "clearance";
  static const char *const LeadToCorner = "lead to corner";
  static const char *const OffSide = "off-side";
  static const char *const Var = "var";
  static const char *const FoulNoCard = "foul (no card)";
  static const char *const StatisticsAndSummary = "statistics and summary";
  static const char *const BallPossession = "ball possession";
  static const char *const BallOutOfPlay = "ball out of play";
}

const std::vector<std::string> &GetCanonicalLabels();
bool IsCanonicalLabel(const std::string &label);

}
}

#endif
