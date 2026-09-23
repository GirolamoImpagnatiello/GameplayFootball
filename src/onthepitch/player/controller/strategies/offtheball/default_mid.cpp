// written by bastiaan konings schuiling 2008 - 2015
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not be used for anything important.
// i do not offer support, so don't ask. to be used for inspiration :)

#include "default_mid.hpp"

#include "../../../../../main.hpp"

DefaultMidfieldStrategy::DefaultMidfieldStrategy(ElizaController *controller) : Strategy(controller) {
  name = "default midfield";
}

DefaultMidfieldStrategy::~DefaultMidfieldStrategy() {
}

void DefaultMidfieldStrategy::RequestInput(const MentalImage *mentalImage, Vector3 &direction, float &velocity) {

  bool offensiveComponents = true;
  bool defensiveComponents = true;
  bool laziness = true;

  Vector3 desiredPosition_static = team->GetController()->GetAdaptedFormationPosition(CastPlayer(), false);
  Vector3 desiredPosition_dynamic = team->GetController()->GetAdaptedFormationPosition(CastPlayer(), true);
  float actionDistance = NormalizedClamp(player->GetPosition().GetDistance(match->GetDesignatedPossessionPlayer()->GetPosition()), 15.0f, 20.0f);
  float staticPositionBias = curve(0.9f * actionDistance, 1.0f); // lower values = swap position with other players' formation positions more easily
  Vector3 desiredPosition = desiredPosition_static * staticPositionBias + desiredPosition_dynamic * (1.0f - staticPositionBias);

  if (offensiveComponents) {
    // support position
    float attackBias = NormalizedClamp((controller->GetFadingTeamPossessionAmount() - 0.5f) * 1.0f, 0.1f, 0.7f);
    bool makeRun = false;
    if (attackBias > 0.9f) {
      if (team->GetController()->GetEndApplyAttackingRun_ms() > match->GetActualTime_ms() && team->GetController()->GetAttackingRunPlayer() == player) {
        makeRun = true;
      }
    }
    Vector3 supportPosition = controller->GetSupportPosition_ForceField(mentalImage, desiredPosition, makeRun);
    desiredPosition = desiredPosition * (1.0f - attackBias) + supportPosition * attackBias;
  }

  if (defensiveComponents) {

    float mindset = AI_GetMindSet(CastPlayer()->GetDynamicFormationEntry().role);
    controller->AddDefensiveComponent(desiredPosition, pow(clamp(1.5f - mindset - controller->GetFadingTeamPossessionAmount(), 0.0f, 1.0f), 0.7f));

    // offside trap (used to be applied before AddDefensiveComponent)
    team->GetController()->ApplyOffsideTrap(desiredPosition);
  }

  const e_PlayerRole role = CastPlayer()->GetDynamicFormationEntry().role;
  if (role == e_PlayerRole_LM || role == e_PlayerRole_RM || role == e_PlayerRole_AM) {
    const float offensiveAggression = clamp(
        GetConfiguration()->GetReal("ai_offensive_aggression", 1.0f), 0.5f, 2.0f);
    const float possessionRunBias = NormalizedClamp(
        controller->GetFadingTeamPossessionAmount(), 0.65f, 1.35f);
    const float ballProgress = NormalizedClamp(
        match->GetBall()->Predict(0).coords[0] * -team->GetSide(), 0.0f, 38.0f);
    const float supportRunBias = clamp(
        possessionRunBias * ballProgress * (0.30f + offensiveAggression * 0.12f),
        0.0f, 0.58f);
    if (supportRunBias > 0.0f) {
      const float ballY = match->GetBall()->Predict(0).coords[1];
      const bool wideFinalThird = ballProgress > 0.52f && fabs(ballY) > 10.0f;
      const float adaptedRunBias = clamp(supportRunBias + (wideFinalThird ? 0.12f : 0.0f), 0.0f, 0.68f);
      Vector3 runTarget((pitchHalfW - (wideFinalThird ? 12.0f : 18.0f)) * -team->GetSide(),
                        wideFinalThird ? clamp(-ballY * 0.35f, -10.0f, 10.0f)
                                       : clamp(desiredPosition.coords[1] * 0.65f, -14.0f, 14.0f),
                        0.0f);
      desiredPosition = desiredPosition * (1.0f - adaptedRunBias) + runTarget * adaptedRunBias;
      team->GetController()->ApplyOffsideTrap(desiredPosition);
    }
  }

  direction = (desiredPosition - player->GetPosition()).GetNormalized(player->GetDirectionVec());
  float desiredVelocity = (desiredPosition - player->GetPosition()).GetLength() * distanceToVelocityMultiplier;

  // laziness
  if (laziness) desiredVelocity = controller->GetLazyVelocity(desiredVelocity);

  desiredVelocity = clamp(desiredVelocity, 0, sprintVelocity);

  velocity = desiredVelocity;
}
