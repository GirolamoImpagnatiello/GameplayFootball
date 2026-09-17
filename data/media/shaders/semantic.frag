#version 150

#pragma optimize(on)

uniform vec3 semanticColor;
uniform sampler2D map_albedo;
uniform int isPitchSurface;
in vec2 frag_texcoord;

out vec4 stdout;

void main(void) {
  vec3 color = semanticColor;
  if (isPitchSurface != 0) {
    // Painted markings are baked into the pitch albedo at match creation.
    // The white paint in overlay.png is semi-transparent. The sRGB pitch
    // texture is decoded to linear values when sampled here.
    vec3 albedo = texture(map_albedo, frag_texcoord).rgb;
    if (min(min(albedo.r, albedo.g), albedo.b) > 0.10)
      color = vec3(1.0, 0.0, 1.0);
  }
  stdout = vec4(color, 1.0);
}
