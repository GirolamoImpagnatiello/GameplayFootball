#version 150

#pragma optimize(on)

uniform vec3 semanticColor;
uniform sampler2D map_albedo;
uniform int isPitchSurface;
uniform int isCrowdSurface;
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
  if (isCrowdSurface != 0) {
    // The stadium crowd is rendered on RGBA billboards. The old semantic
    // pass colored each complete quad as crowd, making the seating bowl look
    // fully occupied even where the texture is transparent. Preserve the
    // people silhouettes and label the remaining billboard area as seats.
    float crowdAlpha = texture(map_albedo, frag_texcoord).a;
    if (crowdAlpha < 0.50)
      color = vec3(0.0, 0.0, 96.0 / 255.0);
  }
  stdout = vec4(color, 1.0);
}
