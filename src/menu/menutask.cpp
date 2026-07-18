// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not be used for anything important.
// i do not offer support, so don't ask. to be used for inspiration :)

#include "menutask.hpp"

#include "../onthepitch/match.hpp"

#include "pagefactory.hpp"

#include "mainmenu.hpp"
#include "ingame/ingame.hpp"
#include "visualoptions.hpp"
#include "ingame/replaymenu.hpp"
#include "ingame/phasemenu.hpp"
#include "ingame/gameover.hpp"

#include "gametask.hpp"

#include "main.hpp"

#include "framework/scheduler.hpp"
#include "managers/resourcemanagerpool.hpp"

using namespace blunted;

void SetActiveController(int side, bool keyboard) {
  bool keyboardActive = true;
  const std::vector<SideSelection> sides = GetMenuTask()->GetControllerSetup();
  int menuControllerID = -1;
  for (unsigned int i = 0; i < sides.size(); i++) {
    if (sides.at(i).side == side) {
      if (GetControllers().at(sides.at(i).controllerID)->GetDeviceType() == e_HIDeviceType_Gamepad) {
        menuControllerID = static_cast<HIDGamepad*>(GetControllers().at(sides.at(i).controllerID))->GetGamepadID();
        keyboardActive = false;
      }
      break;
    }
    if (i == sides.size() - 1) menuControllerID = 0; // AI opponent, so allow choosing their team with controller
  }

  GetMenuTask()->SetActiveJoystickID(menuControllerID);
  if (keyboard) {
    if (keyboardActive) {
      GetMenuTask()->EnableKeyboard();
    } else {
      GetMenuTask()->DisableKeyboard();
    }
  } else {
    GetMenuTask()->EnableKeyboard();
  }
}

MenuTask::MenuTask(float aspectRatio, float margin, TTF_Font *defaultFont, TTF_Font *defaultOutlineFont) : Gui2Task(GetScene2D(), aspectRatio, margin) {

  automaticMatchCount = std::max(0, GetConfiguration()->GetInt("automatic_match_count", 0));
  automaticBatchEnabled = GetConfiguration()->GetBool("automatic_batch_enabled", false) && automaticMatchCount > 0;
  automaticQuitWhenDone = GetConfiguration()->GetBool("automatic_quit_when_done", true);
  automaticMatchesCompleted = 0;

  Gui2Style *style = windowManager->GetStyle();

  style->SetFont(e_TextType_Default, defaultFont);
  style->SetFont(e_TextType_DefaultOutline, defaultOutlineFont);
  style->SetFont(e_TextType_Caption, defaultFont);
  style->SetFont(e_TextType_Title, defaultFont);
  style->SetFont(e_TextType_ToolTip, defaultFont);

/* previous colorset
  style->SetColor(e_DecorationType_Dark1, Vector3(0, 0, 0));
  style->SetColor(e_DecorationType_Dark2, Vector3(63, 63, 63));
  style->SetColor(e_DecorationType_Bright1, Vector3(240, 255, 210));
  style->SetColor(e_DecorationType_Bright2, Vector3(214, 194, 154));
  style->SetColor(e_DecorationType_Toggled, Vector3(255, 20, 70));
*/

  // huisstijl:
  // blauw: 0, 100, 220
  // orange: 240, 100, 0
  style->SetColor(e_DecorationType_Dark1, Vector3(20, 35, 55));
  style->SetColor(e_DecorationType_Dark2, Vector3(60, 35, 20));
  style->SetColor(e_DecorationType_Bright1, Vector3(150, 180, 220));
  style->SetColor(e_DecorationType_Bright2, Vector3(240, 150, 100));
  style->SetColor(e_DecorationType_Toggled, Vector3(240, 60, 60));

  windowManager->SetTimeStep_ms(10);

  Gui2Root *root = windowManager->GetRoot();
  root->Show();

  PageFactory *pageFactory = new PageFactory();
  windowManager->SetPageFactory(pageFactory);

  if (automaticBatchEnabled) {

    const int size = GetControllers().size();
    for (int i = 0; i < size; i++) {
      SideSelection side;
      side.controllerID = i;
      side.controllerImage = 0;
      side.side = 0; // no human-controlled side: AI versus AI
      queuedFixture->sides.push_back(side);
    }

    menuAction = e_MenuAction_AutomaticMatch;

  } else if (!QuickStart()) {

    queuedFixture->team1KitNum = 1;
    queuedFixture->team2KitNum = 2;

    menuAction = e_MenuAction_Menu;

  } else {

    int size = GetControllers().size();
    for (int i = 0; i < size; i++) {
      SideSelection side;
      side.controllerID = i;
      side.controllerImage = 0;
      if ((size > 1 && i == 1) || (size == 1 && i == 0)) {
        side.side = -1;
      } else {
        side.side = 0;
      }
      queuedFixture->sides.push_back(side);
    }

    // 1 == ajax
    // 2 == arsenal
    // 3 == barcelona
    // 4 == bayern
    // 5 == borussia
    // 6 == man utd
    // 7 == psv
    // 8 == real madrid
    queuedFixture->teamID1 = "3";
    queuedFixture->teamID2 = "8";
    queuedFixture->team1KitNum = 2;
    queuedFixture->team2KitNum = 2;

    menuAction = e_MenuAction_Menu;

  }

}

MenuTask::~MenuTask() {
  if (Verbose()) printf("exiting menutask.. ");

  delete windowManager->GetPageFactory();

  if (Verbose()) printf("done\n");
}

void MenuTask::ProcessPhase() {

  Gui2Task::ProcessPhase();

  if (menuAction == e_MenuAction_Menu) {

    windowManager->GetPagePath()->Clear();

    GetGameTask()->Action(e_GameTaskMessage_StopMatch);
    GetGameTask()->Action(e_GameTaskMessage_StartMenuScene);

    Properties properties;
    if (!QuickStart()) {
      if (!IsReleaseVersion()) {
        windowManager->GetPageFactory()->CreatePage((int)e_PageID_MainMenu, properties, 0);
      } else {
        windowManager->GetPageFactory()->CreatePage((int)e_PageID_Intro, properties, 0);
      }
    } else {
      windowManager->GetPageFactory()->CreatePage((int)e_PageID_LoadingMatch, properties, 0);
    }

  } else if (menuAction == e_MenuAction_Game) {

    GetGameTask()->Action(e_GameTaskMessage_StopMenuScene);
    GetGameTask()->Action(e_GameTaskMessage_StartMatch);

  } else if (menuAction == e_MenuAction_AutomaticMatch) {

    windowManager->GetPagePath()->Clear();

    GetGameTask()->Action(e_GameTaskMessage_StopMatch);
    GetGameTask()->Action(e_GameTaskMessage_StopMenuScene);
    GetGameTask()->Action(e_GameTaskMessage_StartMenuScene);

    ConfigureAutomaticFixture();

    Properties properties;
    windowManager->GetPageFactory()->CreatePage((int)e_PageID_LoadingMatch, properties, 0);

    printf("Automatic batch: starting match %i/%i (team %i vs team %i)\n",
           automaticMatchesCompleted + 1, automaticMatchCount, GetTeamID(0), GetTeamID(1));

  } else if (menuAction == e_MenuAction_AutomaticDone) {

    windowManager->GetPagePath()->Clear();
    GetGameTask()->Action(e_GameTaskMessage_StopMatch);
    GetGameTask()->Action(e_GameTaskMessage_StopMenuScene);

    printf("Automatic batch: completed %i/%i matches\n", automaticMatchesCompleted, automaticMatchCount);

    if (automaticQuitWhenDone) {
      EnvironmentManager::GetInstance().SignalQuit();
    } else {
      automaticBatchEnabled = false;
      GetGameTask()->Action(e_GameTaskMessage_StartMenuScene);
      Properties properties;
      windowManager->GetPageFactory()->CreatePage((int)e_PageID_MainMenu, properties, 0);
    }

  }

  menuAction = e_MenuAction_None;
}

void MenuTask::ConfigureAutomaticFixture() {
  DatabaseResult *result = GetDB()->Query("select id from teams order by id");
  std::vector<std::string> teamIDs;
  for (unsigned int i = 0; i < result->data.size(); ++i) {
    if (!result->data.at(i).empty()) teamIDs.push_back(result->data.at(i).at(0));
  }
  delete result;

  if (teamIDs.size() < 2) {
    Log(e_FatalError, "MenuTask", "ConfigureAutomaticFixture", "Automatic batch requires at least two teams in the database");
    return;
  }

  std::string homeTeamID = GetConfiguration()->Get("automatic_home_team_id", "3");
  std::string awayTeamID = GetConfiguration()->Get("automatic_away_team_id", "8");

  if (GetConfiguration()->GetBool("automatic_random_teams", true)) {
    const unsigned int homeIndex = static_cast<unsigned int>(rand()) % teamIDs.size();
    unsigned int awayIndex = static_cast<unsigned int>(rand()) % (teamIDs.size() - 1);
    if (awayIndex >= homeIndex) ++awayIndex;
    homeTeamID = teamIDs.at(homeIndex);
    awayTeamID = teamIDs.at(awayIndex);
  } else {
    if (std::find(teamIDs.begin(), teamIDs.end(), homeTeamID) == teamIDs.end() ||
        std::find(teamIDs.begin(), teamIDs.end(), awayTeamID) == teamIDs.end() ||
        homeTeamID == awayTeamID) {
      Log(e_FatalError, "MenuTask", "ConfigureAutomaticFixture", "Invalid or identical automatic team IDs");
      return;
    }
  }

  queuedFixture.Lock();
  queuedFixture->teamID1 = homeTeamID;
  queuedFixture->teamID2 = awayTeamID;
  queuedFixture->team1KitNum = GetConfiguration()->GetInt("automatic_home_kit", 2);
  queuedFixture->team2KitNum = GetConfiguration()->GetInt("automatic_away_kit", 2);
  queuedFixture.Unlock();
}

void MenuTask::CompleteAutomaticMatch() {
  if (!automaticBatchEnabled) return;

  ++automaticMatchesCompleted;
  if (automaticMatchesCompleted < automaticMatchCount) {
    menuAction = e_MenuAction_AutomaticMatch;
  } else {
    menuAction = e_MenuAction_AutomaticDone;
  }
}

bool MenuTask::QuickStart() {
  return !IsReleaseVersion() && EnvironmentManager::GetInstance().GetTime_ms() < 10000; // after 5 seconds, quickstart disabled (== after > 0 matches have been played)
}

void MenuTask::QuitGame() {
  EnvironmentManager::GetInstance().SignalQuit();
}

void MenuTask::ReleaseAllButtons() {
  // when going back to game, depress all buttons, so we don't go around doing passes we don't want
  for (int joyID = 0; joyID < UserEventManager::GetInstance().GetJoystickCount(); joyID++) {
    for (unsigned int buttonID = 0; buttonID < blunted::_JOYSTICK_MAXBUTTONS; buttonID++) {
      UserEventManager::GetInstance().SetJoyButtonState(joyID, buttonID, false);
    }
  }
  UserEventManager::GetInstance().SetKeyboardState(SDLK_ESCAPE, false);
}
