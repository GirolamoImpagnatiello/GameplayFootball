#ifndef _HPP_ENVIRONMENT_PROFILE
#define _HPP_ENVIRONMENT_PROFILE

#include "base/properties.hpp"
#include "base/math/vector3.hpp"

namespace blunted {

  struct EnvironmentProfile {
    Vector3 sunPosition;
    Vector3 sunColor;
    Vector3 ambientTint;
    Vector3 fogColor;
    Vector3 skyColor;
    Vector3 stadiumLightColor;
    float ambientBrightness;
    float ambientDesaturation;
    float pitchAmbientScale;
    float fogDensity;
    float postBrightness;
    float postContrast;
    float postSaturation;
    float stadiumLightIntensity;
  };

  inline EnvironmentProfile GetEnvironmentProfile(const Properties &configuration) {
    const std::string mode = configuration.Get(
      "match_lighting_resolved",
      configuration.Get("match_lighting", "day"));

    EnvironmentProfile profile;

    // Day deliberately preserves the established daytime rendering.
    profile.sunPosition = Vector3(-1.2f, 0.4f, 1.0f);
    profile.sunColor = Vector3(0.9f, 0.8f, 1.0f) * 1.4f;
    profile.ambientTint = Vector3(0.9f, 1.0f, 1.2f);
    profile.fogColor = Vector3(0.85f, 0.85f, 0.9f);
    profile.skyColor = Vector3(0.85f, 0.85f, 0.9f);
    profile.stadiumLightColor = Vector3(1.0f, 0.96f, 0.88f);
    profile.ambientBrightness = 0.15f;
    profile.ambientDesaturation = 0.7f;
    profile.pitchAmbientScale = 0.65f;
    profile.fogDensity = 1.0f;
    profile.postBrightness = 1.0f;
    profile.postContrast = 0.3f;
    profile.postSaturation = 0.95f;
    profile.stadiumLightIntensity = 0.0f;

    if (mode == "sunset") {
      profile.sunPosition = Vector3(-1.25f, 0.35f, 0.22f);
      profile.sunColor = Vector3(1.55f, 0.72f, 0.38f);
      profile.ambientTint = Vector3(1.12f, 0.86f, 0.82f);
      profile.fogColor = Vector3(0.78f, 0.52f, 0.48f);
      profile.skyColor = Vector3(0.72f, 0.42f, 0.48f);
      profile.ambientBrightness = 0.17f;
      profile.ambientDesaturation = 0.35f;
      profile.pitchAmbientScale = 0.43f;
      profile.fogDensity = 0.9f;
      profile.postBrightness = 0.96f;
      profile.postContrast = 0.26f;
      profile.postSaturation = 1.02f;
      profile.stadiumLightIntensity = 0.08f;
    } else if (mode == "cloudy") {
      profile.sunPosition = Vector3(-0.6f, 0.25f, 1.0f);
      profile.sunColor = Vector3(0.42f, 0.47f, 0.56f);
      profile.ambientTint = Vector3(0.82f, 0.91f, 1.05f);
      profile.fogColor = Vector3(0.62f, 0.66f, 0.72f);
      profile.skyColor = Vector3(0.56f, 0.60f, 0.66f);
      profile.ambientBrightness = 0.24f;
      profile.ambientDesaturation = 0.62f;
      profile.pitchAmbientScale = 0.52f;
      profile.fogDensity = 1.65f;
      profile.postBrightness = 0.93f;
      profile.postContrast = 0.16f;
      profile.postSaturation = 0.76f;
    } else if (mode == "night") {
      // Broadcast-style night match: the pitch is brightly lit by neutral
      // floodlights, while the sky and stadium surroundings remain dark.
      profile.sunPosition = Vector3(-0.45f, -0.65f, 2.4f);
      profile.sunColor = Vector3(0.10f, 0.12f, 0.16f);
      profile.ambientTint = Vector3(0.82f, 0.90f, 1.0f);
      profile.fogColor = Vector3(0.055f, 0.075f, 0.12f);
      profile.skyColor = Vector3(0.018f, 0.028f, 0.065f);
      profile.stadiumLightColor = Vector3(0.92f, 0.96f, 1.0f);
      profile.ambientBrightness = 0.045f;
      profile.ambientDesaturation = 0.28f;
      profile.pitchAmbientScale = 0.30f;
      profile.fogDensity = 0.55f;
      profile.postBrightness = 1.08f;
      profile.postContrast = 0.27f;
      profile.postSaturation = 0.96f;
      profile.stadiumLightIntensity = 0.34f;
    }

    return profile;
  }

}

#endif
