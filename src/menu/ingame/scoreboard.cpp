// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not be used for anything important.
// i do not offer support, so don't ask. to be used for inspiration :)

#include "scoreboard.hpp"

#include "utils/gui2/windowmanager.hpp"

#include <SDL2/SDL.h>

#include "../../onthepitch/match.hpp"

using namespace blunted;

Gui2ScoreBoard::Gui2ScoreBoard(Gui2WindowManager *windowManager, Match *match) : Gui2View(windowManager, "scoreboard", 2, 2, 96, 5.5f), match(match) {
  x_percent = 2;
  y_percent = 2;
  width_percent = 96;
  // Broadcast HUD: increase the complete score strip proportionally so the
  // clock, team names and score remain legible in 480p/720p captures.
  height_percent = 5.5f;

  const Vector3 textColor(245, 248, 252);
  const Vector3 scoreColor(205, 255, 20);
  const Vector3 timeColor(255, 255, 255);
  const Vector3 textOutlineColor(4, 12, 28);

  goalCount[0] = 0;
  goalCount[1] = 0;
  int x, y, w, h;
  windowManager->GetCoordinates(x_percent, y_percent, width_percent, height_percent, x, y, w, h);

  // Clear OCR-friendly broadcast hierarchy:
  // TIME | HOME NAME | HOME SCORE | LEAGUE | AWAY SCORE | AWAY NAME
  const float barWidth = 47.0f;
  const float timeX = 1.2f;
  const float homeNameX = 10.0f;
  const float homeScoreX = 21.0f;
  const float leagueX = 25.8f;
  const float awayScoreX = 31.5f;
  const float awayNameX = 35.0f;
  content_yOffset = 0.2f;

  Gui2Image *bg = new Gui2Image(windowManager, "image_scoreboard_bg", 0, 0, barWidth, height_percent);
  bg->LoadImage("media/menu/scoreboard_bg_broadcast_v2.png");
  this->AddView(bg);
  bg->Show();

  leagueLogo = new Gui2Image(windowManager, "game_scoreboard_leaguelogo", leagueX, 0,
                             height_percent / windowManager->GetAspectRatio(), height_percent);
  this->AddView(leagueLogo);
  leagueLogo->LoadImage("media/menu/league.png"); // todo: actual league picca
  leagueLogo->Show();

  timeCaption = new Gui2Caption(windowManager, "game_scoreboard_timecaption", timeX, 0, 6.0f, height_percent * 0.9f, "0:00");
  teamNameCaption[0] = new Gui2Caption(windowManager, "game_scoreboard_team1name", homeNameX, 0, 8.5f, height_percent * 0.9f, match->GetTeam(0)->GetTeamData()->GetShortName());
  teamNameCaption[1] = new Gui2Caption(windowManager, "game_scoreboard_team2name", awayNameX, 0, 8.5f, height_percent * 0.9f, match->GetTeam(1)->GetTeamData()->GetShortName());
  goalCountCaption[0] = new Gui2Caption(windowManager, "game_scoreboard_team1goals", homeScoreX, 0, 3.0f, height_percent * 0.9f, "0");
  goalCountCaption[1] = new Gui2Caption(windowManager, "game_scoreboard_team2goals", awayScoreX, 0, 3.0f, height_percent * 0.9f, "0");

  timeCaption->SetColor(timeColor);
  timeCaption->SetOutlineColor(textOutlineColor);
  teamNameCaption[0]->SetColor(textColor);
  teamNameCaption[0]->SetOutlineColor(textOutlineColor);
  teamNameCaption[1]->SetColor(textColor);
  teamNameCaption[1]->SetOutlineColor(textOutlineColor);
  goalCountCaption[0]->SetColor(scoreColor);
  goalCountCaption[0]->SetOutlineColor(textOutlineColor);
  goalCountCaption[1]->SetColor(scoreColor);
  goalCountCaption[1]->SetOutlineColor(textOutlineColor);

  // Keep the unrelated broadcaster logo compact while the score strip grows.
  const float tvLogoHeight = 5.2f;
  tvLogo = new Gui2Image(windowManager, "game_scoreboard_tvlogo", width_percent - (tvLogoHeight * 2.0f) / windowManager->GetAspectRatio(), 0, (tvLogoHeight * 2.0f) / windowManager->GetAspectRatio(), tvLogoHeight);
  this->AddView(tvLogo);
  tvLogo->LoadImage("media/menu/tvlogo.png");
  tvLogo->Show();

  this->AddView(timeCaption);
  timeCaption->Show();
  this->AddView(teamNameCaption[0]);
  teamNameCaption[0]->Show();
  this->AddView(teamNameCaption[1]);
  teamNameCaption[1]->Show();
  this->AddView(goalCountCaption[0]);
  goalCountCaption[0]->Show();
  this->AddView(goalCountCaption[1]);
  goalCountCaption[1]->Show();

  SetGoalCount(0, 0);
  SetGoalCount(1, 0);
  LayoutContent();

  this->Show();
}

Gui2ScoreBoard::~Gui2ScoreBoard() {
  /* will be cleaned up while deleting gui2 tree automatically
  bg->Exit();
  delete bg;
  leagueLogo->Exit();
  delete leagueLogo;
  timeCaption->Exit();
  delete timeCaption;
  teamNameCaption[0]->Exit();
  delete teamNameCaption[0];
  teamNameCaption[1]->Exit();
  delete teamNameCaption[1];
  goalCountCaption[0]->Exit();
  delete goalCountCaption[0];
  goalCountCaption[1]->Exit();
  delete goalCountCaption[1];
  teamLogo[0]->Exit();
  delete teamLogo[0];
  teamLogo[1]->Exit();
  delete teamLogo[1];
  tvLogo->Exit();
  delete tvLogo;
  */
}

void Gui2ScoreBoard::GetImages(std::vector < boost::intrusive_ptr<Image2D> > &target) {
  Gui2View::GetImages(target);
}

void Gui2ScoreBoard::Redraw() {
}

void Gui2ScoreBoard::LayoutContent() {
  // Centre every dynamic text surface inside a stable semantic slot. Caption
  // widgets resize to their rendered text, so fixed left coordinates make
  // short team names and one-digit scores look visibly unbalanced.
  const float contentY = (height_percent - height_percent * 0.9f) * 0.5f;
  const float timeCenter = 3.9f;
  const float homeNameCenter = 14.4f;
  const float homeScoreCenter = 22.2f;
  const float competitionCenter = 27.4f;
  const float awayScoreCenter = 32.6f;
  const float awayNameCenter = 40.2f;

  timeCaption->SetPosition(timeCenter - timeCaption->GetTextWidthPercent() * 0.5f,
                           contentY);
  teamNameCaption[0]->SetPosition(
      homeNameCenter - teamNameCaption[0]->GetTextWidthPercent() * 0.5f,
      contentY);
  goalCountCaption[0]->SetPosition(
      homeScoreCenter - goalCountCaption[0]->GetTextWidthPercent() * 0.5f,
      contentY);
  goalCountCaption[1]->SetPosition(
      awayScoreCenter - goalCountCaption[1]->GetTextWidthPercent() * 0.5f,
      contentY);
  teamNameCaption[1]->SetPosition(
      awayNameCenter - teamNameCaption[1]->GetTextWidthPercent() * 0.5f,
      contentY);

  const float logoHeight = 5.0f;
  const float logoWidth = logoHeight / windowManager->GetAspectRatio();
  leagueLogo->SetSize(logoWidth, logoHeight);
  leagueLogo->SetPosition(competitionCenter - logoWidth * 0.5f,
                          (height_percent - logoHeight) * 0.5f);
}

void Gui2ScoreBoard::SetTimeStr(const std::string &timeStr) {
  this->timeStr = timeStr;
  timeCaption->SetCaption(timeStr);
  LayoutContent();
}

void Gui2ScoreBoard::SetGoalCount(int teamID, int goalCount) {
  this->goalCount[teamID] = goalCount;
  goalCountCaption[teamID]->SetCaption(int_to_str(goalCount));
  LayoutContent();
}
